#include "PacketAuthentication.h"
#include "../Generated/PacketEnvelope.pb.h"

#include <windows.h>
#include <bcrypt.h>
#include <limits>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace
{
	class HmacSha256Provider final
	{
	public:
		HmacSha256Provider() noexcept
		{
			if (BCryptOpenAlgorithmProvider(
				&algorithmHandle,
				BCRYPT_SHA256_ALGORITHM,
				nullptr,
				BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0)
			{
				return;
			}

			ULONG resultSize = 0;
			if (BCryptGetProperty(
				algorithmHandle,
				BCRYPT_OBJECT_LENGTH,
				reinterpret_cast<PUCHAR>(&hashObjectSize),
				sizeof(hashObjectSize),
				&resultSize,
				0) < 0
				|| BCryptGetProperty(
					algorithmHandle,
					BCRYPT_HASH_LENGTH,
					reinterpret_cast<PUCHAR>(&hashSize),
					sizeof(hashSize),
					&resultSize,
					0) < 0)
			{
				BCryptCloseAlgorithmProvider(algorithmHandle, 0);
				algorithmHandle = nullptr;
			}
		}

		~HmacSha256Provider()
		{
			if (algorithmHandle != nullptr)
			{
				BCryptCloseAlgorithmProvider(algorithmHandle, 0);
			}
		}

		HmacSha256Provider(const HmacSha256Provider&) = delete;
		HmacSha256Provider& operator=(const HmacSha256Provider&) = delete;

		bool Compute(
			const cmudp::protocol::AuthenticationKey& inKey,
			const std::string_view inData,
			cmudp::protocol::AuthenticationTag& outTag) const
		{
			if (algorithmHandle == nullptr
				|| hashSize != outTag.size()
				|| inData.size() > (std::numeric_limits<ULONG>::max)())
			{
				return false;
			}

			std::vector<uint8_t> hashObject(hashObjectSize);
			BCRYPT_HASH_HANDLE hashHandle = nullptr;
			if (BCryptCreateHash(
				algorithmHandle,
				&hashHandle,
				hashObject.data(),
				static_cast<ULONG>(hashObject.size()),
				const_cast<PUCHAR>(inKey.data()),
				static_cast<ULONG>(inKey.size()),
				0) < 0)
			{
				return false;
			}

			const auto destroyHash = [&hashHandle]() noexcept
				{
					BCryptDestroyHash(hashHandle);
				};
			if (BCryptHashData(
				hashHandle,
				reinterpret_cast<PUCHAR>(const_cast<char*>(inData.data())),
				static_cast<ULONG>(inData.size()),
				0) < 0
				|| BCryptFinishHash(
					hashHandle,
					outTag.data(),
					static_cast<ULONG>(outTag.size()),
					0) < 0)
			{
				destroyHash();
				return false;
			}

			destroyHash();
			return true;
		}

	private:
		BCRYPT_ALG_HANDLE algorithmHandle = nullptr;
		ULONG hashObjectSize = 0;
		ULONG hashSize = 0;
	};

	const HmacSha256Provider& GetHmacSha256Provider() noexcept
	{
		static const HmacSha256Provider provider;
		return provider;
	}
}

namespace cmudp::protocol
{
	bool GenerateAuthenticationKey(AuthenticationKey& outKey) noexcept
	{
		if (BCryptGenRandom(
			nullptr,
			outKey.data(),
			static_cast<ULONG>(outKey.size()),
			BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
		{
			outKey.fill(0);
			return false;
		}
		return true;
	}

	bool IsValidAuthenticationKey(const AuthenticationKey& inKey) noexcept
	{
		uint8_t combinedValue = 0;
		for (const uint8_t byte : inKey)
		{
			combinedValue |= byte;
		}
		return combinedValue != 0;
	}

	bool ComputeAuthenticationTag(
		const AuthenticationKey& inKey,
		const std::string_view inAuthenticatedData,
		AuthenticationTag& outTag) noexcept
	{
		if (not IsValidAuthenticationKey(inKey))
		{
			return false;
		}
		try
		{
			return GetHmacSha256Provider().Compute(inKey, inAuthenticatedData, outTag);
		}
		catch (...)
		{
			outTag.fill(0);
			return false;
		}
	}

	bool VerifyAuthenticationTag(
		const AuthenticationKey& inKey,
		const std::string_view inAuthenticatedData,
		const std::string_view inAuthenticationTag) noexcept
	{
		if (inAuthenticationTag.size() != AUTHENTICATION_TAG_SIZE)
		{
			return false;
		}

		AuthenticationTag expectedTag{};
		if (not ComputeAuthenticationTag(inKey, inAuthenticatedData, expectedTag))
		{
			return false;
		}

		uint8_t difference = 0;
		for (size_t i = 0; i < expectedTag.size(); ++i)
		{
			difference |= expectedTag[i]
				^ static_cast<uint8_t>(inAuthenticationTag[i]);
		}
		return difference == 0;
	}

	bool SerializeAuthenticatedPacket(
		const uint64_t connectionId,
		const uint64_t sequence,
		const uint32_t packetType,
		const std::string_view inPayload,
		const AuthenticationKey& inKey,
		std::string& outPacketData)
	{
		outPacketData.clear();
		if (connectionId == 0
			|| sequence == 0
			|| packetType == 0
			|| inPayload.size() > (std::numeric_limits<int>::max)()
			|| not IsValidAuthenticationKey(inKey))
		{
			return false;
		}

		AuthenticatedPacket authenticatedPacket;
		authenticatedPacket.set_protocol_version(CURRENT_PROTOCOL_VERSION);
		authenticatedPacket.set_connection_id(connectionId);
		authenticatedPacket.set_sequence(sequence);
		authenticatedPacket.set_packet_type(packetType);
		authenticatedPacket.set_payload(inPayload.data(), inPayload.size());

		std::string authenticatedData;
		if (not authenticatedPacket.SerializeToString(&authenticatedData))
		{
			return false;
		}

		AuthenticationTag authenticationTag{};
		if (not ComputeAuthenticationTag(inKey, authenticatedData, authenticationTag))
		{
			return false;
		}

		PacketEnvelope envelope;
		envelope.set_connection_id(connectionId);
		envelope.set_authenticated_data(std::move(authenticatedData));
		envelope.set_authentication_tag(
			reinterpret_cast<const char*>(authenticationTag.data()),
			authenticationTag.size());

		if (envelope.ByteSizeLong() > MAX_PACKET_SIZE)
		{
			return false;
		}
		return envelope.SerializeToString(&outPacketData);
	}
}
