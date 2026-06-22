#pragma once
#include "ProcessorBase.h"
#include "TickerTask.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

class TickerProcessor : public ProcessorBase
{
public:
	TickerProcessor() = delete;
	explicit TickerProcessor(ProcessorManager& inProcessorManager, const int inTickMillisecond);
	~TickerProcessor() override;

public:
	bool StartImpl() override;
	void StopImpl() override;

private:
	struct TickerTaskCompare
	{
		bool operator()(const std::shared_ptr<TickerTask>& lhs, const std::shared_ptr<TickerTask>& rhs) const
		{
			return lhs->GetFireTime() > rhs->GetFireTime();
		}
	};

	using TickerTaskQueue = std::priority_queue<
		std::shared_ptr<TickerTask>,
		std::vector<std::shared_ptr<TickerTask>>,
		TickerTaskCompare>;

private:
	void RunTickerThread();
	void ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task) override;

private:
	int tickMillisecond;
	TickerTaskQueue tickerTaskQueue;
	std::mutex tickerTaskQueueMutex;
	std::condition_variable tickerTaskQueueCV;
	std::mutex tickerCallbackMutex;
};
