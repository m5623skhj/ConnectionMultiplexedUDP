#pragma once

#include "ProcessorTaskBase.h"
#include "ProcessorType.h"
#include <winsock2.h>
#include <vector>

class ReceivedPacketTask final : public ProcessorTaskBase
{
public:
	ReceivedPacketTask(
		const ProcessorIndex inIoProcessorIndex,
		const sockaddr_in& inSenderAddress,
		const char* inPacketData,
		const size_t inPacketSize)
		: ioProcessorIndex(inIoProcessorIndex)
		, senderAddress(inSenderAddress)
		, packetData(inPacketData, inPacketData + inPacketSize)
	{
	}
	~ReceivedPacketTask() override = default;

public:
	ProcessorIndex GetIoProcessorIndex() const
	{
		return ioProcessorIndex;
	}

	const sockaddr_in& GetSenderAddress() const
	{
		return senderAddress;
	}

	const std::vector<char>& GetPacketData() const
	{
		return packetData;
	}

private:
	ProcessorIndex ioProcessorIndex = InvalidProcessor;
	sockaddr_in senderAddress{};
	std::vector<char> packetData;
};
