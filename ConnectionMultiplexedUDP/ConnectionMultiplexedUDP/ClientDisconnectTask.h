#pragma once
#include "ProcessorTaskBase.h"
#include "SessionLookupTable.h"
#include <chrono>

class ClientDisconnectTask final : public ProcessorTaskBase
{
public:
	explicit ClientDisconnectTask(const ConnectionId inConnectionId)
		: connectionId(inConnectionId)
	{
	}

	ClientDisconnectTask(
		const ConnectionId inConnectionId,
		const std::chrono::steady_clock::time_point inRequestedTime,
		const std::chrono::milliseconds inTimeout)
		: connectionId(inConnectionId)
		, requestedTime(inRequestedTime)
		, timeout(inTimeout)
	{
	}
	~ClientDisconnectTask() override = default;

public:
	ConnectionId GetConnectionId() const
	{
		return connectionId;
	}

	bool RequiresTimeoutValidation() const
	{
		return timeout > std::chrono::milliseconds::zero();
	}

	std::chrono::steady_clock::time_point GetRequestedTime() const
	{
		return requestedTime;
	}

	std::chrono::milliseconds GetTimeout() const
	{
		return timeout;
	}

private:
	ConnectionId connectionId = InvalidConnectionId;
	std::chrono::steady_clock::time_point requestedTime{};
	std::chrono::milliseconds timeout{ 0 };
};
