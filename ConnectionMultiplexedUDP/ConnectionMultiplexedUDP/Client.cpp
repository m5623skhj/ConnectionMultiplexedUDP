#include "Client.h"

ClientId Client::GetClientId() const
{
	return clientId;
}

bool Client::SendPacket()
{
    return true;
}
