#pragma once
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>

class ProcessorTask
{
};

class ProcessorBase
{
public:
	ProcessorBase() = default;
	virtual ~ProcessorBase() = default;

public:
	virtual bool Start();
	virtual void Stop();

public:
	void PushTaskToProcessor(std::unique_ptr<ProcessorTask>&& task);

private:
	virtual void ProcessTask(std::unique_ptr<ProcessorTask>&& task) = 0;
	void RunProcessTaskThread();

protected:
	std::jthread processorThread;
	std::atomic_bool needStop = false;

private:
	std::jthread processTaskThread;

	std::queue<std::unique_ptr<ProcessorTask>> taskQueue;
	std::mutex messageQueueMutex;
	std::condition_variable messageQueueCV;
};