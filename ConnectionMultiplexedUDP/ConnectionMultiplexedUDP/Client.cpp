#include "Client.h"
#include "ClientPacketSender.h"

ClientId Client::GetClientId() const
{
	return clientId;
}

bool Client::SendPacket(
	const ConnectionId inConnectionId,
	const PacketType inPacketType,
	const std::string_view inPayload)
{
	if (clientId == InvalidClientId
		|| packetSender == nullptr
		|| inConnectionId == InvalidConnectionId
		|| inPacketType == INVALID_PACKET_TYPE)
	{
		return false;
	}

	return packetSender->SendPacket(
		clientId,
		inConnectionId,
		inPacketType,
		inPayload);
}
