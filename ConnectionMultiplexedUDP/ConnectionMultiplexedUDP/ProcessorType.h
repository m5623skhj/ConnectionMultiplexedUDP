#pragma once

enum class EProcessorType : uint8_t
{
	IO,
	Logic,

	Max,
};

using ProcessorIndex = unsigned char;
inline constexpr ProcessorIndex AnyProcessor = 0xFF;
inline constexpr ProcessorIndex InvalidProcessor = 0xFF;