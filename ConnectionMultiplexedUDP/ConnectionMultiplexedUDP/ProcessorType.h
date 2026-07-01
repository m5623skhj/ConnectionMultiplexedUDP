#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

enum class EProcessorType : uint8_t
{
	IO,
	Logic,
	TICKER,
	HEARTBEAT,

	Max,
};

using ProcessorIndex = size_t;
inline constexpr ProcessorIndex AnyProcessor = (std::numeric_limits<ProcessorIndex>::max)();
inline constexpr ProcessorIndex InvalidProcessor = AnyProcessor;
