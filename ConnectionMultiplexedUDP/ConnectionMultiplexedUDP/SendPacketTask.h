#pragma once

#include "ProcessorTaskBase.h"
#include <string_view>
#include <vector>
#include <winsock2.h>

class SendPacketTask final : public ProcessorTaskBase
{
public:
	SendPacketTask(
		const sockaddr_in& inDestAddress,
		const std::string_view inPacketData)
		: destAddress(inDestAddress)
		, packetData(inPacketData.begin(), inPacketData.end())
	{
	}
	~SendPacketTask() override = default;

public:
	const sockaddr_in& GetDestAddress() const
	{
		return destAddress;
	}

	const std::vector<char>& GetPacketData() const
	{
		return packetData;
	}

private:
	sockaddr_in destAddress{};
	std::vector<char> packetData;
};
