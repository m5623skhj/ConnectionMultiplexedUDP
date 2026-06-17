#include "LogicProcessor.h"

LogicProcessor::LogicProcessor()
{
}

LogicProcessor::~LogicProcessor()
{
}

bool LogicProcessor::Start()
{
	processorThread = std::jthread(&LogicProcessor::RunLogicThread, this);

	return true;
}

void LogicProcessor::Stop()
{
	if (processorThread.joinable())
	{
		processorThread.request_stop();
	}
}

void LogicProcessor::RunLogicThread()
{
}
