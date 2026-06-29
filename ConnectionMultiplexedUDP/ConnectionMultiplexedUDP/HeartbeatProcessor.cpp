#include "HeartbeatProcessor.h"
#include "ProcessorManager.h"

#include <chrono>
#include <thread>

namespace
{
	constexpr std::chrono::milliseconds HEARTBEAT_SCAN_INTERVAL{ 1000 };
	constexpr std::chrono::milliseconds SESSION_TIMEOUT{ 30000 };
}

HeartbeatProcessor::HeartbeatProcessor(ProcessorManager& inProcessorManager)
	: ProcessorBase(inProcessorManager)
{
}

HeartbeatProcessor::~HeartbeatProcessor()
{
	Stop();
}

bool HeartbeatProcessor::StartImpl()
{
	StartProcessorThread([this] { RunHeartbeatThread(); });

	return true;
}

void HeartbeatProcessor::StopImpl()
{
}

void HeartbeatProcessor::RunHeartbeatThread()
{
	while (not needStop.load())
	{
		std::this_thread::sleep_for(HEARTBEAT_SCAN_INTERVAL);
		if (needStop.load())
		{
			break;
		}

		processorManager.RequestTimedOutSessionDisconnects(SESSION_TIMEOUT);
	}
}

void HeartbeatProcessor::ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task)
{
}
