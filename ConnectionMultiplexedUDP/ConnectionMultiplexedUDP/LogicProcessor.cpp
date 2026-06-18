#include "LogicProcessor.h"

LogicProcessor::LogicProcessor(ProcessorManager& inProcessorManager)
    : ProcessorBase(inProcessorManager)
{
}

LogicProcessor::~LogicProcessor()
{
}

bool LogicProcessor::StartImpl()
{
	processorThread = std::jthread(&LogicProcessor::RunLogicThread, this);

	return true;
}

void LogicProcessor::StopImpl()
{
}

void LogicProcessor::RunLogicThread()
{
}

void LogicProcessor::ProcessTask(std::unique_ptr<ProcessorTask>&& task)
{
}
