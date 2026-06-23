#pragma once

#include "Client.h"
#include "Protocol/PacketAuthentication.h"
#include <cstdint>
#include <mutex>
#include <string_view>
#include <winsock2.h>

class Session final
{
public:
	Session(
		ClientId inClientId,
		const sockaddr_in& inRemoteAddress,
		const cmudp::protocol::AuthenticationKey& inAuthenticationKey);
	~Session();

	ClientId GetClientId() const noexcept;
	bool VerifyPacketAuthentication(
		const sockaddr_in& inSenderAddress,
		std::string_view inAuthenticatedData,
		std::string_view inAuthenticationTag) const noexcept;
	bool AcceptSequence(uint64_t sequence) noexcept;

private:
	ClientId clientId;
	sockaddr_in remoteAddress;
	cmudp::protocol::AuthenticationKey authenticationKey;

	std::mutex replayMutex;
	uint64_t highestReceivedSequence = 0;
	uint64_t receivedSequenceWindow = 0;
};
