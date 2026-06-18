#include "ProcessorBase.h"

ProcessorBase::ProcessorBase(ProcessorManager& inProcessorManager)
    : processorManager(inProcessorManager)
{
}

bool ProcessorBase::Start()
{
    if (not StartImpl())
    {
        return false;
	}

    processTaskThread = std::jthread(&ProcessorBase::RunProcessTaskThread, this);

    return true;
}

void ProcessorBase::Stop()
{
    if (needStop.exchange(true))
    {
        return;
    }

	StopImpl();

	if (processorThread.joinable())
	{
		processorThread.join();
	}

	if (processTaskThread.joinable())
	{
		messageQueueCV.notify_all();
		processTaskThread.join();
	}
}

void ProcessorBase::PushTaskToProcessor(std::unique_ptr<ProcessorTask>&& task)
{
    if (needStop.load())
    {
        return;
    }

	{
		std::scoped_lock lock(messageQueueMutex);
		taskQueue.push(std::move(task));
	}

	messageQueueCV.notify_one();
}

size_t ProcessorBase::GetTaskQueueSize() const
{
	std::scoped_lock lock(messageQueueMutex);
	return taskQueue.size();
}

void ProcessorBase::RunProcessTaskThread()
{
    while (true)
    {
        std::unique_lock lock(messageQueueMutex);
        messageQueueCV.wait(lock, [this]
            {
                return not taskQueue.empty() || needStop;
            });

        if (needStop && taskQueue.empty())
        {
            return;
        }

        while (not taskQueue.empty())
        {
            auto task = std::move(taskQueue.front());
            taskQueue.pop();

            lock.unlock();
            ProcessTask(std::move(task));
            lock.lock();
        }
    }
}