#include "ProcessorBase.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

ProcessorBase::ProcessorBase(ProcessorManager& inProcessorManager)
    : processorManager(inProcessorManager)
{
}

bool ProcessorBase::Start()
{
	{
		std::scoped_lock lifecycleLock(lifecycleMutex);
		if (state.load() != EState::Stopped)
		{
			return false;
		}

		needStop.store(false);
		state.store(EState::Starting);
	}

	try
    {
		if (not StartImpl())
		{
			throw std::runtime_error("processor failed to start");
		}

		processTaskThread = std::jthread(&ProcessorBase::RunProcessTaskThread, this);
	}
	catch (...)
	{
		{
			std::scoped_lock queueLock(messageQueueMutex);
			state.store(EState::Stopping);
			needStop.store(true);
		}

		StopImpl();
		StopThreads();
		state.store(EState::Stopped);
		return false;
	}

	state.store(EState::Running);

    return true;
}

bool ProcessorBase::Stop()
{
	{
		std::scoped_lock lifecycleLock(lifecycleMutex);
		const EState currentState = state.load();
		if (currentState == EState::Stopped)
		{
			return true;
		}
		if (currentState != EState::Running || IsCurrentThread())
		{
			return false;
		}

		std::scoped_lock queueLock(messageQueueMutex);
		state.store(EState::Stopping);
		needStop.store(true);
	}

	StopImpl();
	StopThreads();
	state.store(EState::Stopped);
	return true;
}

bool ProcessorBase::IsCurrentThread() const
{
	std::scoped_lock identityLock(workerIdentityMutex);
	const std::thread::id currentThreadId = std::this_thread::get_id();
	return processorThreadId == currentThreadId || processTaskThreadId == currentThreadId;
}

void ProcessorBase::StartProcessorThread(std::function<void()> inFunction)
{
	processorThread = std::jthread(
		&ProcessorBase::RunProcessorThread,
		this,
		std::move(inFunction));
}

void ProcessorBase::RunProcessorThread(std::function<void()> function)
{
	{
		std::scoped_lock identityLock(workerIdentityMutex);
		processorThreadId = std::this_thread::get_id();
	}

	try
	{
		function();
	}
	catch (const std::exception& exception)
	{
		std::cerr << "processor thread failed: " << exception.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "processor thread failed with an unknown exception\n";
	}

	{
		std::scoped_lock identityLock(workerIdentityMutex);
		processorThreadId = {};
	}
}

void ProcessorBase::StopThreads()
{
	if (processorThread.joinable())
	{
		processorThread.join();
	}
	StopAfterProcessorThreadImpl();

	if (processTaskThread.joinable())
	{
		messageQueueCV.notify_all();
		processTaskThread.join();
	}

	std::scoped_lock queueLock(messageQueueMutex);
	while (not taskQueue.empty())
	{
		taskQueue.pop();
	}
}

void ProcessorBase::StopAfterProcessorThreadImpl()
{
}

bool ProcessorBase::PushTaskToProcessor(std::unique_ptr<ProcessorTaskBase>&& task)
{
	if (task == nullptr)
	{
		return false;
	}

	{
		std::scoped_lock lock(messageQueueMutex);
		const EState currentState = state.load();
		if (currentState != EState::Starting && currentState != EState::Running)
		{
			return false;
		}

		taskQueue.push(std::move(task));
	}

	messageQueueCV.notify_one();
	return true;
}

size_t ProcessorBase::GetTaskQueueSize() const
{
	std::scoped_lock lock(messageQueueMutex);
	return taskQueue.size();
}

void ProcessorBase::RunProcessTaskThread()
{
	{
		std::scoped_lock identityLock(workerIdentityMutex);
		processTaskThreadId = std::this_thread::get_id();
	}

    while (true)
    {
        std::unique_lock lock(messageQueueMutex);
        messageQueueCV.wait(lock, [this]
            {
                return not taskQueue.empty() || needStop;
            });

        if (needStop && taskQueue.empty())
        {
			break;
        }

        while (not taskQueue.empty())
        {
            auto task = std::move(taskQueue.front());
            taskQueue.pop();

            lock.unlock();
			try
			{
				ProcessTask(std::move(task));
			}
			catch (const std::exception& exception)
			{
				std::cerr << "processor task failed: " << exception.what() << '\n';
			}
			catch (...)
			{
				std::cerr << "processor task failed with an unknown exception\n";
			}
			lock.lock();
        }
    }

	{
		std::scoped_lock identityLock(workerIdentityMutex);
		processTaskThreadId = {};
	}
}
