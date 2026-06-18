#pragma once
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>

class ProcessorManager;

class ProcessorTask
{
};

class ProcessorBase
{
public:
	ProcessorBase() = delete;
	explicit ProcessorBase(ProcessorManager& inProcessorManager);
	virtual ~ProcessorBase() = default;

public:
	virtual bool Start() final;
	virtual void Stop() final;

private:
	virtual bool StartImpl() = 0;
	virtual void StopImpl() = 0;

public:
	void PushTaskToProcessor(std::unique_ptr<ProcessorTask>&& task);
	size_t GetTaskQueueSize() const;

private:
	virtual void ProcessTask(std::unique_ptr<ProcessorTask>&& task) = 0;
	void RunProcessTaskThread();

protected:
	std::jthread processorThread;
	std::atomic_bool needStop = false;

	ProcessorManager& processorManager;

private:
	std::jthread processTaskThread;

	std::queue<std::unique_ptr<ProcessorTask>> taskQueue;
	mutable std::mutex messageQueueMutex;
	std::condition_variable messageQueueCV;
};