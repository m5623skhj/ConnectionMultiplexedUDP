#pragma once
#include "ClientPacketSender.h"
#include "ClientManager.h"
#include "ProcessorManager.h"
#include "Protocol/PacketAuthentication.h"
#include <cstdint>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

class ServerCore final : public ClientPacketSender
{
public:
	ServerCore(
		const int inIoProcessorCount, 
		const int inLogicProcessorCount, 
		const uint16_t ioProcessorPortBase,
		const int inTickMillisecond,
		const int inSessionTimeoutMillisecond = 30000
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
	bool SendPacket(
		ClientId inClientId,
		ConnectionId inConnectionId,
		PacketType inPacketType,
		std::string_view inPayload) override;

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
