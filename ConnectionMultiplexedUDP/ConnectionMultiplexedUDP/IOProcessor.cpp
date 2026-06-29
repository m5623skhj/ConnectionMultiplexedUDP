#include "IOProcessor.h"
#include "ProcessorManager.h"
#include "Protocol/PacketProtocol.h"
#include "ReceivedPacketTask.h"
#include "SendPacketTask.h"
#include <array>
#include <iostream>
#include <limits>
#include <utility>

IOProcessor::IOProcessor(
	ProcessorManager& inProcessorManager,
	const ProcessorIndex inProcessorIndex,
	const uint16_t inPort)
	: ProcessorBase(inProcessorManager)
	, processorIndex(inProcessorIndex)
	, sock(INVALID_SOCKET)
	, port(inPort)
{
}

IOProcessor::~IOProcessor()
{
	Stop();
}

bool IOProcessor::StartImpl()
{
	const SOCKET newSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (newSocket == INVALID_SOCKET)
	{
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		return false;
	}

	constexpr DWORD RECEIVE_TIMEOUT_MS = 100;
	if (setsockopt(
		newSocket,
		SOL_SOCKET,
		SO_RCVTIMEO,
		reinterpret_cast<const char*>(&RECEIVE_TIMEOUT_MS),
		sizeof(RECEIVE_TIMEOUT_MS)) == SOCKET_ERROR)
	{
		std::cout << "setsockopt failed with error: " << WSAGetLastError() << std::endl;
		closesocket(newSocket);
		return false;
	}

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (bind(newSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
	{
		std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
		closesocket(newSocket);
		return false;
	}

	{
		std::scoped_lock lock(socketMutex);
		sock = newSocket;
	}

	StartProcessorThread([this] { RunRecvThread(); });
	return true;
}

void IOProcessor::StopImpl()
{
}

void IOProcessor::StopAfterProcessorThreadImpl()
{
	SOCKET socketToClose = INVALID_SOCKET;
	{
		std::scoped_lock lock(socketMutex);
		socketToClose = std::exchange(sock, INVALID_SOCKET);
	}

	if (socketToClose != INVALID_SOCKET)
	{
		closesocket(socketToClose);
	}
}

bool IOProcessor::SendPacket(const sockaddr_in& destAddr, const char* data, const int dataSize)
{
	std::scoped_lock lock(socketMutex);
	if (needStop.load() || sock == INVALID_SOCKET)
	{
		return false;
	}

	const int result = sendto(sock, data, dataSize, 0, reinterpret_cast<const sockaddr*>(&destAddr), sizeof(destAddr));
	if (result == SOCKET_ERROR)
	{
		std::cout << "sendto failed with error: " << WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

void IOProcessor::RunRecvThread()
{
	std::array<char, cmudp::protocol::MAX_PACKET_SIZE> recvBuffer;

	while (true)
	{
		SOCKET receiveSocket = INVALID_SOCKET;
		{
			std::scoped_lock lock(socketMutex);
			receiveSocket = sock;
		}

		if (receiveSocket == INVALID_SOCKET)
		{
			break;
		}

		sockaddr_in addr{};
		int addrLen = sizeof(addr);
		const int result = recvfrom(
			receiveSocket,
			recvBuffer.data(),
			static_cast<int>(recvBuffer.size()),
			0,
			reinterpret_cast<sockaddr*>(&addr),
			&addrLen);

		if (result == SOCKET_ERROR)
		{
			const int errorCode = WSAGetLastError();
			if (needStop.load())
			{
				break;
			}

			if (errorCode != WSAETIMEDOUT)
			{
				std::cout << "recvfrom failed : " << errorCode << '\n';
			}
			continue;
		}

		auto task = std::make_unique<ReceivedPacketTask>(
			processorIndex,
			addr,
			recvBuffer.data(),
			static_cast<size_t>(result));
		const uint64_t endpointAffinityKey =
			(static_cast<uint64_t>(addr.sin_addr.s_addr) << 16)
			| static_cast<uint64_t>(addr.sin_port);
		processorManager.PushTaskToProcessorByAffinity(
			EProcessorType::Logic,
			std::move(task),
			endpointAffinityKey);
	}
}

void IOProcessor::ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task)
{
	auto* sendPacketTask = dynamic_cast<SendPacketTask*>(task.get());
	if (sendPacketTask == nullptr)
	{
		return;
	}

	const std::vector<char>& packetData = sendPacketTask->GetPacketData();
	if (packetData.empty() || packetData.size() > (std::numeric_limits<int>::max)())
	{
		return;
	}

	SendPacket(
		sendPacketTask->GetDestAddress(),
		packetData.data(),
		static_cast<int>(packetData.size()));
}
