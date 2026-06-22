#include "TickerProcessor.h"

TickerProcessor::TickerProcessor(ProcessorManager& inProcessorManager, const int inTickMillisecond)
	: ProcessorBase(inProcessorManager)
	, tickMillisecond(inTickMillisecond)
{
}

TickerProcessor::~TickerProcessor()
{
}

bool TickerProcessor::StartImpl()
{
	processorThread = std::jthread(&TickerProcessor::RunTickerThread, this);

	return true;
}

void TickerProcessor::StopImpl()
{
}

void TickerProcessor::RunTickerThread()
{
	using clock = std::chrono::steady_clock;
	auto nextTickTime = clock::now();

	while (not needStop.load())
	{
		nextTickTime += std::chrono::milliseconds(tickMillisecond);
		std::this_thread::sleep_until(nextTickTime);

		if (needStop.load())
		{
			break;
		}

		// Tick 처리
	}
}

void TickerProcessor::ProcessTask(std::unique_ptr<ProcessorTask>&& task)
{
}
