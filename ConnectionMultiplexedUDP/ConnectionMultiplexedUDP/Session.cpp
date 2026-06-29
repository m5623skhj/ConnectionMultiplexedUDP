#include "Session.h"
#include <limits>
#include <windows.h>

Session::Session(
	const ClientId inClientId,
	const sockaddr_in& inRemoteAddress,
	const cmudp::protocol::AuthenticationKey& inAuthenticationKey)
	: clientId(inClientId)
	, remoteAddress(inRemoteAddress)
	, authenticationKey(inAuthenticationKey)
	, lastReceivedTime(std::chrono::steady_clock::now())
{
}

Session::~Session()
{
	SecureZeroMemory(authenticationKey.data(), authenticationKey.size());
}

ClientId Session::GetClientId() const noexcept
{
	return clientId;
}

void Session::MarkReceivedNow()
{
	std::scoped_lock lock(activityMutex);
	lastReceivedTime = std::chrono::steady_clock::now();
}

bool Session::HasTimedOut(
	const std::chrono::steady_clock::time_point inNow,
	const std::chrono::milliseconds inTimeout) const
{
	if (inTimeout <= std::chrono::milliseconds::zero())
	{
		return false;
	}

	std::scoped_lock lock(activityMutex);
	return inNow - lastReceivedTime >= inTimeout;
}

sockaddr_in Session::GetRemoteAddress() const noexcept
{
	return remoteAddress;
}

bool Session::BuildSendPacket(
	const ConnectionId inConnectionId,
	const PacketType inPacketType,
	const std::string_view inPayload,
	std::string& outPacketData)
{
	if (inConnectionId == InvalidConnectionId
		|| inPacketType == INVALID_PACKET_TYPE)
	{
		outPacketData.clear();
		return false;
	}

	std::scoped_lock lock(sendMutex);
	const uint64_t sequence = nextSendSequence;
	if (sequence == 0 || sequence == (std::numeric_limits<uint64_t>::max)())
	{
		outPacketData.clear();
		return false;
	}

	if (not cmudp::protocol::SerializeAuthenticatedPacket(
			inConnectionId,
			sequence,
			inPacketType,
			inPayload,
			authenticationKey,
			outPacketData))
	{
		return false;
	}

	++nextSendSequence;
	return true;
}

bool Session::VerifyPacketAuthentication(
	const sockaddr_in& inSenderAddress,
	const std::string_view inAuthenticatedData,
	const std::string_view inAuthenticationTag) const noexcept
{
	if (inSenderAddress.sin_family != remoteAddress.sin_family
		|| inSenderAddress.sin_addr.s_addr != remoteAddress.sin_addr.s_addr
		|| inSenderAddress.sin_port != remoteAddress.sin_port)
	{
		return false;
	}

	return cmudp::protocol::VerifyAuthenticationTag(
			authenticationKey,
			inAuthenticatedData,
			inAuthenticationTag);
}

bool Session::AcceptSequence(const uint64_t sequence) noexcept
{
	if (sequence == 0)
	{
		return false;
	}

	std::scoped_lock lock(replayMutex);
	constexpr uint64_t REPLAY_WINDOW_SIZE = 64;
	if (highestReceivedSequence == 0)
	{
		highestReceivedSequence = sequence;
		receivedSequenceWindow = 1;
		return true;
	}

	if (sequence > highestReceivedSequence)
	{
		const uint64_t advance = sequence - highestReceivedSequence;
		receivedSequenceWindow = advance >= REPLAY_WINDOW_SIZE
			? 1
			: (receivedSequenceWindow << advance) | 1;
		highestReceivedSequence = sequence;
		return true;
	}

	const uint64_t distance = highestReceivedSequence - sequence;
	if (distance >= REPLAY_WINDOW_SIZE)
	{
		return false;
	}

	const uint64_t sequenceBit = uint64_t{ 1 } << distance;
	if ((receivedSequenceWindow & sequenceBit) != 0)
	{
		return false;
	}

	receivedSequenceWindow |= sequenceBit;
	return true;
}
