#pragma once
#include <atomic>
#include <cstdint>
#include <thread>
#include <queue>
#include <condition_variable>
#include <functional>
#include <mutex>
#include "ProcessorTaskBase.h"

class ProcessorManager;

class ProcessorBase
{
public:
	ProcessorBase() = delete;
	explicit ProcessorBase(ProcessorManager& inProcessorManager);
	virtual ~ProcessorBase() = default;

public:
	virtual bool Start() final;
	virtual bool Stop() final;
	bool IsCurrentThread() const;

private:
	virtual bool StartImpl() = 0;
	virtual void StopImpl() = 0;
	virtual void StopAfterProcessorThreadImpl();

public:
	bool PushTaskToProcessor(std::unique_ptr<ProcessorTaskBase>&& task);
	size_t GetTaskQueueSize() const;

private:
	enum class EState : uint8_t
	{
		Stopped,
		Starting,
		Running,
		Stopping,
	};

	virtual void ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task) = 0;
	void RunProcessorThread(std::function<void()> function);
	void RunProcessTaskThread();
	void StopThreads();

protected:
	void StartProcessorThread(std::function<void()> inFunction);

	std::jthread processorThread;
	std::atomic_bool needStop = false;

	ProcessorManager& processorManager;

private:
	std::mutex lifecycleMutex;
	std::atomic<EState> state = EState::Stopped;
	std::jthread processTaskThread;
	mutable std::mutex workerIdentityMutex;
	std::thread::id processorThreadId;
	std::thread::id processTaskThreadId;

	std::queue<std::unique_ptr<ProcessorTaskBase>> taskQueue;
	mutable std::mutex messageQueueMutex;
	std::condition_variable messageQueueCV;
};
