#pragma once
#include "ClientManager.h"
#include "ProcessorManager.h"
#include "Protocol/PacketAuthentication.h"
#include <cstdint>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

class ServerCore
{
public:
	ServerCore(
		const int inIoProcessorCount, 
		const int inLogicProcessorCount, 
		const uint16_t ioProcessorPortBase,
		const int inTickMillisecond
	);
	~ServerCore();

public:
	bool Start();
	bool Stop();
	ClientId AddClient(std::unique_ptr<Client> inClient);
	bool RemoveClient(ClientId clientId);
	ConnectionId RegisterClientSession(
		ClientId clientId,
		const sockaddr_in& inRemoteAddress,
		const cmudp::protocol::AuthenticationKey& inAuthenticationKey);
	bool RemoveClientSession(ConnectionId connectionId);

private:
	enum class EState : uint8_t
	{
		Stopped,
		Starting,
		Running,
		Stopping,
	};

	std::mutex lifecycleMutex;
	std::mutex clientSessionMutex;
	ClientManager clientManager;
	ProcessorManager processorManager;
	EState state = EState::Stopped;
};
