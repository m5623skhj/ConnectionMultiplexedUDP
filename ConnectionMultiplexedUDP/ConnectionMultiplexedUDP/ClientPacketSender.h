#pragma once

#include "Client.h"
#include <string_view>

class ClientPacketSender
{
public:
	virtual ~ClientPacketSender() = default;

public:
	virtual bool SendPacket(
		ClientId inClientId,
		ConnectionId inConnectionId,
		PacketType inPacketType,
		std::string_view inPayload) = 0;
};
