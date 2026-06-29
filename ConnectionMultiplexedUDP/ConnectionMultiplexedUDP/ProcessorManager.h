#pragma once
#include "ProcessorBase.h"
#include "ClientManager.h"
#include "Protocol/PacketAuthentication.h"
#include "SessionLookupTable.h"
#include <atomic>
#include <array>
#include <cstdint>
#include <mutex>
#include <vector>
#include <string_view>
#include <winsock2.h>
#include "ProcessorType.h"

class ProcessorManager
{
public:
	ProcessorManager() = delete;
	explicit ProcessorManager(
		ClientManager& inClientManager,
		const int inIoProcessorCount, 
		const int inLogicProcessorCount, 
		const uint16_t ioProcessorPortBase,
		const int inTickMillisecond
	);
	~ProcessorManager();

public:
	bool Start();
	bool Stop();

public:
	bool PushTaskToProcessor(const EProcessorType processorType, std::unique_ptr<ProcessorTaskBase>&& task, const ProcessorIndex c = AnyProcessor);
	bool PushTaskToProcessorByAffinity(
		EProcessorType processorType,
		std::unique_ptr<ProcessorTaskBase>&& task,
		uint64_t affinityKey);
	bool SendPacket(
		ProcessorIndex ioProcessorIndex,
		const sockaddr_in& inDestAddress,
		std::string_view inPacketData);
	ConnectionId RegisterClientSession(
		ClientId clientId,
		const sockaddr_in& inRemoteAddress,
		const cmudp::protocol::AuthenticationKey& inAuthenticationKey);
	bool RemoveClientSession(ConnectionId connectionId);
	size_t RemoveClientSessions(ClientId clientId);
	bool AuthenticateAndDispatchPacket(
		const sockaddr_in& inSenderAddress,
		ConnectionId connectionId,
		std::string_view inAuthenticatedData,
		std::string_view inAuthenticationTag) const;

private:
	enum class EState : uint8_t
	{
		Stopped,
		Starting,
		Running,
		Stopping,
	};

	void RegisterAllProcessor();
	void StopAllProcessors();
	ProcessorIndex GetLeastBusyProcessorIndex(const EProcessorType processorType) const;

private:
	ClientManager& clientManager;
	SessionLookupTable sessionLookupTable;
	int ioProcessorCount;
	int logicProcessorCount;
	uint16_t ioProcessorPortBase;
	int tickerProcessorIntervalMs;
	bool configurationValid = false;
	
private:
	mutable std::mutex lifecycleMutex;
	std::atomic<EState> state = EState::Stopped;
	std::array<std::vector<std::unique_ptr<ProcessorBase>>, static_cast<size_t>(EProcessorType::Max)> processorGroup;
};
