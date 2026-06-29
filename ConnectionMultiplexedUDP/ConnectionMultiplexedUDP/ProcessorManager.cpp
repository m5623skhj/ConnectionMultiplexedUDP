#include "ProcessorManager.h"
#include "ClientDisconnectTask.h"
#include "IOProcessor.h"
#include "LogicProcessor.h"
#include "TickerProcessor.h"
#include "HeartbeatProcessor.h"
#include "Generated/PacketEnvelope.pb.h"
#include "SendPacketTask.h"
#include "Session.h"
#include <limits>
#include <string>

ProcessorManager::ProcessorManager(
	ClientManager& inClientManager,
	const int inIoProcessorCount,
	const int inLogicProcessorCount,
	const uint16_t inIoProcessorPortBase,
	const int inTickerProcessorIntervalMs,
	const int inSessionTimeoutMillisecond)
	: clientManager(inClientManager)
	, sessionLookupTable(1000)
	, ioProcessorCount(inIoProcessorCount)
	, logicProcessorCount(inLogicProcessorCount)
	, ioProcessorPortBase(inIoProcessorPortBase)
	, tickerProcessorIntervalMs(inTickerProcessorIntervalMs)
	, sessionTimeoutMs(inSessionTimeoutMillisecond)
{
	const uint32_t availablePortCount =
		static_cast<uint32_t>((std::numeric_limits<uint16_t>::max)())
		- static_cast<uint32_t>(ioProcessorPortBase) + 1;
	configurationValid = ioProcessorCount > 0
		&& logicProcessorCount > 0
		&& tickerProcessorIntervalMs > 0
		&& sessionTimeoutMs > 0
		&& static_cast<uint32_t>(ioProcessorCount) <= availablePortCount;
	if (configurationValid)
	{
		RegisterAllProcessor();
	}
}

ProcessorManager::~ProcessorManager()
{
	Stop();
}

bool ProcessorManager::Start()
{
	{
		std::scoped_lock lifecycleLock(lifecycleMutex);
		if (state.load() != EState::Stopped || not configurationValid)
		{
			return false;
		}

		state.store(EState::Starting);
	}

	constexpr std::array startOrder{
		EProcessorType::Logic,
		EProcessorType::TICKER,
		EProcessorType::HEART_BAET,
		EProcessorType::IO };

	try
	{
		for (const EProcessorType processorType : startOrder)
		{
			for (auto& processor : processorGroup[static_cast<size_t>(processorType)])
			{
				if (not processor->Start())
				{
					state.store(EState::Stopping);
					StopAllProcessors();
					state.store(EState::Stopped);
					return false;
				}
			}
		}
	}
	catch (...)
	{
		state.store(EState::Stopping);
		StopAllProcessors();
		state.store(EState::Stopped);
		return false;
	}

	state.store(EState::Running);
	return true;
}

bool ProcessorManager::Stop()
{
	{
		std::scoped_lock lifecycleLock(lifecycleMutex);
		const EState currentState = state.load();
		if (currentState == EState::Stopped)
		{
			return true;
		}
		if (currentState != EState::Running)
		{
			return false;
		}

		for (const auto& processors : processorGroup)
		{
			for (const auto& processor : processors)
			{
				if (processor->IsCurrentThread())
				{
					return false;
				}
			}
		}

		state.store(EState::Stopping);
	}

	StopAllProcessors();
	state.store(EState::Stopped);
	return true;
}

bool ProcessorManager::PushTaskToProcessor(const EProcessorType processorType, std::unique_ptr<ProcessorTaskBase>&& task, const ProcessorIndex inProcessorIndex)
{
	const EState currentState = state.load();
	if ((currentState != EState::Starting && currentState != EState::Running)
		|| processorType >= EProcessorType::Max
		|| task == nullptr)
	{
		return false;
	}

	ProcessorIndex processorIndex = InvalidProcessor;
	if (inProcessorIndex != AnyProcessor)
	{
		if (inProcessorIndex >= processorGroup[static_cast<size_t>(processorType)].size())
		{
			return false;
		}

		processorIndex = inProcessorIndex;
	}
	else
	{
		ProcessorIndex leastBusyProcessorIndex = GetLeastBusyProcessorIndex(processorType);
		if (leastBusyProcessorIndex == InvalidProcessor)
		{
			return false;
		}

		processorIndex = leastBusyProcessorIndex;
	}
	return processorGroup[static_cast<size_t>(processorType)][processorIndex]->PushTaskToProcessor(std::move(task));
}

bool ProcessorManager::PushTaskToProcessorByAffinity(
	const EProcessorType processorType,
	std::unique_ptr<ProcessorTaskBase>&& task,
	const uint64_t affinityKey)
{
	if (processorType >= EProcessorType::Max)
	{
		return false;
	}

	const auto& processors = processorGroup[static_cast<size_t>(processorType)];
	if (processors.empty())
	{
		return false;
	}

	const ProcessorIndex processorIndex = static_cast<ProcessorIndex>(affinityKey % processors.size());
	return PushTaskToProcessor(processorType, std::move(task), processorIndex);
}

bool ProcessorManager::RequestClientDisconnect(const ConnectionId connectionId)
{
	if (connectionId == InvalidConnectionId)
	{
		return false;
	}

	auto task = std::make_unique<ClientDisconnectTask>(connectionId);
	return PushTaskToProcessorByAffinity(
		EProcessorType::Logic,
		std::move(task),
		connectionId);
}

size_t ProcessorManager::RequestTimedOutSessionDisconnects(const std::chrono::milliseconds sessionTimeout)
{
	if (sessionTimeout <= std::chrono::milliseconds::zero())
	{
		return 0;
	}

	const auto currentTime = std::chrono::steady_clock::now();
	size_t requestedCount = 0;
	for (const auto& [connectionId, session] : sessionLookupTable.SnapshotActiveSessions())
	{
		if (session == nullptr || not session->HasTimedOut(currentTime, sessionTimeout))
		{
			continue;
		}

		auto task = std::make_unique<ClientDisconnectTask>(
			connectionId,
			currentTime,
			sessionTimeout);
		if (PushTaskToProcessorByAffinity(
				EProcessorType::Logic,
				std::move(task),
				connectionId))
		{
			++requestedCount;
		}
	}

	return requestedCount;
}

bool ProcessorManager::SendPacket(
	const ProcessorIndex ioProcessorIndex,
	const sockaddr_in& inDestAddress,
	const std::string_view inPacketData)
{
	if (inDestAddress.sin_family != AF_INET
		|| inPacketData.empty()
		|| inPacketData.size() > cmudp::protocol::MAX_PACKET_SIZE)
	{
		return false;
	}

	auto task = std::make_unique<SendPacketTask>(inDestAddress, inPacketData);
	return PushTaskToProcessor(
		EProcessorType::IO,
		std::move(task),
		ioProcessorIndex);
}

bool ProcessorManager::SendAuthenticatedPacket(
	const ClientId clientId,
	const ConnectionId connectionId,
	const PacketType inPacketType,
	const std::string_view inPayload)
{
	if (clientId == InvalidClientId
		|| connectionId == InvalidConnectionId
		|| inPacketType == INVALID_PACKET_TYPE)
	{
		return false;
	}

	const std::shared_ptr<Session> session = sessionLookupTable.Find(connectionId);
	if (session == nullptr || session->GetClientId() != clientId)
	{
		return false;
	}

	std::string packetData;
	if (not session->BuildSendPacket(
			connectionId,
			inPacketType,
			inPayload,
			packetData))
	{
		return false;
	}

	return SendPacket(
		AnyProcessor,
		session->GetRemoteAddress(),
		packetData);
}

ConnectionId ProcessorManager::RegisterClientSession(
	const ClientId clientId,
	const sockaddr_in& inRemoteAddress,
	const cmudp::protocol::AuthenticationKey& inAuthenticationKey)
{
	if (clientId == InvalidClientId
		|| not cmudp::protocol::IsValidAuthenticationKey(inAuthenticationKey))
	{
		return InvalidConnectionId;
	}

	return sessionLookupTable.Allocate(std::make_shared<Session>(
		clientId,
		inRemoteAddress,
		inAuthenticationKey));
}

bool ProcessorManager::RemoveClientSession(const ConnectionId connectionId)
{
	return sessionLookupTable.Release(connectionId);
}

size_t ProcessorManager::RemoveClientSessions(const ClientId clientId)
{
	return sessionLookupTable.ReleaseByClientId(clientId);
}

std::shared_ptr<Session> ProcessorManager::FindSession(const ConnectionId connectionId) const
{
	return sessionLookupTable.Find(connectionId);
}

bool ProcessorManager::AuthenticateAndDispatchPacket(
	const sockaddr_in& inSenderAddress,
	const ConnectionId connectionId,
	const std::string_view inAuthenticatedData,
	const std::string_view inAuthenticationTag)
{
	const std::shared_ptr<Session> session = sessionLookupTable.Find(connectionId);
	if (session == nullptr
		|| not session->VerifyPacketAuthentication(
			inSenderAddress,
			inAuthenticatedData,
			inAuthenticationTag))
	{
		return false;
	}

	cmudp::protocol::AuthenticatedPacket authenticatedPacket;
	if (not authenticatedPacket.ParseFromArray(
			inAuthenticatedData.data(),
			static_cast<int>(inAuthenticatedData.size()))
		|| authenticatedPacket.connection_id() != connectionId
		|| authenticatedPacket.protocol_version() != cmudp::protocol::CURRENT_PROTOCOL_VERSION
		|| authenticatedPacket.packet_type() == INVALID_PACKET_TYPE
		|| not session->AcceptSequence(authenticatedPacket.sequence()))
	{
		return false;
	}

	session->MarkReceivedNow();
	if (authenticatedPacket.packet_type() == cmudp::protocol::HEARTBEAT_PACKET_TYPE)
	{
		return true;
	}
	if (authenticatedPacket.packet_type() == cmudp::protocol::DISCONNECT_PACKET_TYPE)
	{
		return RequestClientDisconnect(connectionId);
	}

	return clientManager.DispatchPacket(
		session->GetClientId(),
		authenticatedPacket.packet_type(),
		authenticatedPacket.payload());
}

void ProcessorManager::RegisterAllProcessor()
{
	processorGroup[static_cast<size_t>(EProcessorType::IO)].reserve(ioProcessorCount);
	processorGroup[static_cast<size_t>(EProcessorType::Logic)].reserve(logicProcessorCount);

	for (int i = 0; i < ioProcessorCount; ++i)
	{
		processorGroup[static_cast<size_t>(EProcessorType::IO)].push_back(
			std::make_unique<IOProcessor>(*this, static_cast<ProcessorIndex>(i), static_cast<uint16_t>(ioProcessorPortBase + i)));
	}

	for (int i = 0; i < logicProcessorCount; ++i)
	{
		processorGroup[static_cast<size_t>(EProcessorType::Logic)].push_back(std::make_unique<LogicProcessor>(*this));
	}

	processorGroup[static_cast<size_t>(EProcessorType::TICKER)].push_back(std::make_unique<TickerProcessor>(*this, tickerProcessorIntervalMs));
	processorGroup[static_cast<size_t>(EProcessorType::HEART_BAET)].push_back(
		std::make_unique<HeartbeatProcessor>(
			*this,
			std::chrono::milliseconds(sessionTimeoutMs)));
}

void ProcessorManager::StopAllProcessors()
{
	constexpr std::array stopOrder{
		EProcessorType::IO,
		EProcessorType::TICKER,
		EProcessorType::HEART_BAET,
		EProcessorType::Logic };

	for (const EProcessorType processorType : stopOrder)
	{
		for (auto& processor : processorGroup[static_cast<size_t>(processorType)])
		{
			processor->Stop();
		}
	}
}

ProcessorIndex ProcessorManager::GetLeastBusyProcessorIndex(const EProcessorType processorType) const
{
	if (processorType >= EProcessorType::Max)
	{
		return InvalidProcessor;
	}

	std::pair<ProcessorIndex, size_t> leastBusyProcessor{
		InvalidProcessor,
		(std::numeric_limits<size_t>::max)() };
	for (ProcessorIndex i = 0; i < processorGroup[static_cast<size_t>(processorType)].size(); ++i)
	{
		const auto& processor = processorGroup[static_cast<size_t>(processorType)][i];
		const size_t taskQueueSize = processor->GetTaskQueueSize();
		if (taskQueueSize == 0)
		{
			return i;
		}

		if (taskQueueSize < leastBusyProcessor.second)
		{
			leastBusyProcessor.first = i;
			leastBusyProcessor.second = taskQueueSize;
		}
	}

	return leastBusyProcessor.first;
}
