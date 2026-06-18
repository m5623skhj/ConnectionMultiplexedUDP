#include "LogicProcessor.h"

LogicProcessor::LogicProcessor()
{
}

LogicProcessor::~LogicProcessor()
{
}

bool LogicProcessor::Start()
{
	if (not ProcessorBase::Start())
	{
		return false;
	}

	processorThread = std::jthread(&LogicProcessor::RunLogicThread, this);

	return true;
}

void LogicProcessor::Stop()
{
	ProcessorBase::Stop();
}

void LogicProcessor::RunLogicThread()
{
}

void LogicProcessor::ProcessTask(std::unique_ptr<ProcessorTask>&& task)
{
}
