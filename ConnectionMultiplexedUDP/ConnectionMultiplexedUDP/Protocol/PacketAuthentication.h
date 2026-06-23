#pragma once

#include "PacketProtocol.h"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace cmudp::protocol
{
	using AuthenticationKey = std::array<uint8_t, AUTHENTICATION_KEY_SIZE>;
	using AuthenticationTag = std::array<uint8_t, AUTHENTICATION_TAG_SIZE>;

	bool GenerateAuthenticationKey(AuthenticationKey& outKey) noexcept;
	bool IsValidAuthenticationKey(const AuthenticationKey& inKey) noexcept;
	bool ComputeAuthenticationTag(
		const AuthenticationKey& inKey,
		std::string_view inAuthenticatedData,
		AuthenticationTag& outTag) noexcept;
	bool VerifyAuthenticationTag(
		const AuthenticationKey& inKey,
		std::string_view inAuthenticatedData,
		std::string_view inAuthenticationTag) noexcept;

	// Creates the exact UDP payload expected by the server receive path.
	bool SerializeAuthenticatedPacket(
		uint64_t connectionId,
		uint64_t sequence,
		uint32_t packetType,
		std::string_view inPayload,
		const AuthenticationKey& inKey,
		std::string& outPacketData);
}
