#include "ProcessorBase.h"

bool ProcessorBase::Start()
{
    processTaskThread = std::jthread(&ProcessorBase::RunProcessTaskThread, this);

    return true;
}

void ProcessorBase::Stop()
{
	needStop.store(true);

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