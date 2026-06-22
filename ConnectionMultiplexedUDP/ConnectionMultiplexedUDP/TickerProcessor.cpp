#include "TickerProcessor.h"
#include <exception>
#include <iostream>

TickerProcessor::TickerProcessor(ProcessorManager& inProcessorManager, const int inTickMillisecond)
	: ProcessorBase(inProcessorManager)
	, tickMillisecond(inTickMillisecond)
{
}

TickerProcessor::~TickerProcessor()
{
	Stop();
}

bool TickerProcessor::StartImpl()
{
	if (tickMillisecond <= 0)
	{
		return false;
	}

	StartProcessorThread([this] { RunTickerThread(); });

	return true;
}

void TickerProcessor::StopImpl()
{
	{
		std::scoped_lock lock(tickerTaskQueueMutex);
		tickerTaskQueue = TickerTaskQueue{};
	}
	{
		std::scoped_lock callbackLock(tickerCallbackMutex);
	}

	tickerTaskQueueCV.notify_all();
}

void TickerProcessor::RunTickerThread()
{
	using Clock = std::chrono::steady_clock;
	auto nextTickTime = Clock::now();

	std::vector<std::shared_ptr<TickerTask>> tasksToFire;
	const size_t taskReserveSize = 128;
	tasksToFire.reserve(taskReserveSize);

	while (not needStop.load())
	{
		nextTickTime += std::chrono::milliseconds(tickMillisecond);
		{
			std::unique_lock lock(tickerTaskQueueMutex);
			tickerTaskQueueCV.wait_until(lock, nextTickTime, [this]
				{
					return needStop.load();
				});

			if (needStop.load())
			{
				break;
			}

			const auto currentTime = Clock::now();
			while (not tickerTaskQueue.empty() && tickerTaskQueue.top()->GetFireTime() <= currentTime)
			{
				tasksToFire.push_back(tickerTaskQueue.top());
				tickerTaskQueue.pop();
			}
		}

		for (const auto& task : tasksToFire)
		{
			std::scoped_lock callbackLock(tickerCallbackMutex);
			if (needStop.load())
			{
				break;
			}

			try
			{
				task->Fire();
			}
			catch (const std::exception& exception)
			{
				std::cerr << "ticker task failed: " << exception.what() << '\n';
			}
			catch (...)
			{
				std::cerr << "ticker task failed with an unknown exception\n";
			}
		}
		tasksToFire.clear();
	}
}

void TickerProcessor::ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task)
{
	auto* tickerTask = dynamic_cast<TickerTask*>(task.get());
	if (tickerTask == nullptr)
	{
		return;
	}

	std::unique_ptr<TickerTask> ownedTickerTask(static_cast<TickerTask*>(task.release()));
	auto scheduledTask = std::shared_ptr<TickerTask>(std::move(ownedTickerTask));
	{
		std::scoped_lock lock(tickerTaskQueueMutex);
		if (needStop.load())
		{
			return;
		}

		tickerTaskQueue.push(std::move(scheduledTask));
	}
}
