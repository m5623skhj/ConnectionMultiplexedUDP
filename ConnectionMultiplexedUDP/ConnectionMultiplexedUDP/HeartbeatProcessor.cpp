#include "HeartbeatProcessor.h"

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
}

void HeartbeatProcessor::ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task)
{
}
