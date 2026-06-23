#include "Session.h"
#include <windows.h>

Session::Session(
	const ClientId inClientId,
	const sockaddr_in& inRemoteAddress,
	const cmudp::protocol::AuthenticationKey& inAuthenticationKey)
	: clientId(inClientId)
	, remoteAddress(inRemoteAddress)
	, authenticationKey(inAuthenticationKey)
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
