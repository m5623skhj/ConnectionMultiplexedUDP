#pragma once

#include "Client.h"
#include "ConnectionId.h"
#include "Protocol/PacketAuthentication.h"
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
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
	void MarkReceivedNow();
	bool HasTimedOut(
		std::chrono::steady_clock::time_point inNow,
		std::chrono::milliseconds inTimeout) const;
	sockaddr_in GetRemoteAddress() const noexcept;
	bool BuildSendPacket(
		ConnectionId inConnectionId,
		PacketType inPacketType,
		std::string_view inPayload,
		std::string& outPacketData);
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

	mutable std::mutex activityMutex;
	std::chrono::steady_clock::time_point lastReceivedTime;

	std::mutex sendMutex;
	uint64_t nextSendSequence = 1;
};
