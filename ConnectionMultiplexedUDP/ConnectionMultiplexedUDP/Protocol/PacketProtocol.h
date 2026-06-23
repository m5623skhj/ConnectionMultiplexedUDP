#pragma once

#include <cstddef>
#include <cstdint>

namespace cmudp::protocol
{
	inline constexpr uint32_t CURRENT_PROTOCOL_VERSION = 1;
	inline constexpr size_t MAX_PACKET_SIZE = 4096;
	inline constexpr size_t AUTHENTICATION_KEY_SIZE = 32;
	inline constexpr size_t AUTHENTICATION_TAG_SIZE = 32;
}
