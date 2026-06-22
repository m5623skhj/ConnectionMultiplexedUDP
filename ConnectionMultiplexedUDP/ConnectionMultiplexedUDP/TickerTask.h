#pragma once

#include "ProcessorTaskBase.h"
#include <chrono>
#include <functional>
#include <utility>

class TickerTask final : public ProcessorTaskBase
{
public:
	using Clock = std::chrono::steady_clock;
	using FireTime = Clock::time_point;
	using Callback = std::function<void()>;

public:
	TickerTask(const FireTime inFireTime, Callback inCallback)
		: fireTime(inFireTime)
		, callback(std::move(inCallback))
	{
	}
	~TickerTask() override = default;

public:
	FireTime GetFireTime() const
	{
		return fireTime;
	}

	void Fire()
	{
		if (callback)
		{
			callback();
		}
	}

private:
	FireTime fireTime;
	Callback callback;
};
