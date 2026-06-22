#include "LogicProcessor.h"
#include "ReceivedPacketTask.h"

LogicProcessor::LogicProcessor(ProcessorManager& inProcessorManager)
    : ProcessorBase(inProcessorManager)
{
}

LogicProcessor::~LogicProcessor()
{
	Stop();
}

bool LogicProcessor::StartImpl()
{
	return true;
}

void LogicProcessor::StopImpl()
{
}

void LogicProcessor::ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task)
{
	auto* receivedPacketTask = dynamic_cast<ReceivedPacketTask*>(task.get());
	if (receivedPacketTask == nullptr)
	{
		return;
	}

	ProcessReceivedPacket(*receivedPacketTask);
}

void LogicProcessor::ProcessReceivedPacket(const ReceivedPacketTask& task)
{
}
