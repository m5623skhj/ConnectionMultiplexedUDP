#include "HeartbeatProcessor.h"
#include "ProcessorManager.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace
{
	constexpr std::chrono::milliseconds MIN_HEARTBEAT_SCAN_INTERVAL{ 100 };
	constexpr std::chrono::milliseconds MAX_HEARTBEAT_SCAN_INTERVAL{ 1000 };
}

HeartbeatProcessor::HeartbeatProcessor(
	ProcessorManager& inProcessorManager,
	const std::chrono::milliseconds inSessionTimeout)
	: ProcessorBase(inProcessorManager)
	, sessionTimeout(inSessionTimeout)
{
}

HeartbeatProcessor::~HeartbeatProcessor()
{
	Stop();
}

bool HeartbeatProcessor::StartImpl()
{
	if (sessionTimeout <= std::chrono::milliseconds::zero())
	{
		return false;
	}

	StartProcessorThread([this] { RunHeartbeatThread(); });

	return true;
}

void HeartbeatProcessor::StopImpl()
{
}

void HeartbeatProcessor::RunHeartbeatThread()
{
	const std::chrono::milliseconds scanInterval = (std::min)(
		(std::max)(sessionTimeout / 2, MIN_HEARTBEAT_SCAN_INTERVAL),
		MAX_HEARTBEAT_SCAN_INTERVAL);
	while (not needStop.load())
	{
		std::this_thread::sleep_for(scanInterval);
		if (needStop.load())
		{
			break;
		}

		processorManager.RequestTimedOutSessionDisconnects(sessionTimeout);
	}
}

void HeartbeatProcessor::ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task)
{
}
