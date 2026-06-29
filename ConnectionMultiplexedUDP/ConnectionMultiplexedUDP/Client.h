#pragma once

#include "ConnectionId.h"
#include <cstdint>
#include <string_view>

using ClientId = uint64_t;
inline constexpr ClientId InvalidClientId = 0;
using PacketType = uint32_t;
inline constexpr PacketType INVALID_PACKET_TYPE = 0;

class ClientManager;
class ClientPacketSender;

class Client
{
public:
	Client() = default;
	virtual ~Client() = default;

public:
	ClientId GetClientId() const;
	bool SendPacket(
		ConnectionId inConnectionId,
		PacketType inPacketType,
		std::string_view inPayload);

protected:
	virtual void OnClientCreated() noexcept = 0;
	virtual void OnClientDestroyed() noexcept = 0;
	// inPayload is valid only for the duration of this callback.
	virtual void OnRecvPacket(PacketType inPacketType, std::string_view inPayload) = 0;

private:
	friend class ClientManager;

	ClientId clientId = InvalidClientId;
	ClientPacketSender* packetSender = nullptr;
};
