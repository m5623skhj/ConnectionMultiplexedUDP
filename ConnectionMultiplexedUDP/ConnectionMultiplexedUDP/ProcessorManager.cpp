#include "ProcessorManager.h"

ProcessorManager::ProcessorManager(
	int inIoProcessorCount, 
	int inLogicProcessorCount)
	: ioProcessorCount(inIoProcessorCount)
	, logicProcessorCount(inLogicProcessorCount)
{
	processorGroup[static_cast<size_t>(EProcessorType::IO)].reserve(ioProcessorCount);
	processorGroup[static_cast<size_t>(EProcessorType::Logic)].reserve(logicProcessorCount);
}

ProcessorManager::~ProcessorManager()
{
}

bool ProcessorManager::Start()
{
	for (auto& processors : processorGroup)
	{
		for (auto& processor : processors)
		{
			if (not processor->Start())
			{
				return false;
			}
		}
	}

	return true;
}

void ProcessorManager::Stop()
{
	for (auto& processors : processorGroup)
	{
		for (auto& processor : processors)
		{
			processor->Stop();
		}
	}
}

bool ProcessorManager::PushTaskToProcessor(const EProcessorType processorType, std::unique_ptr<ProcessorTask>&& task, const ProcessorIndex inProcessorIndex)
{
	if (processorType >= EProcessorType::Max)
	{
		return false;
	}

	ProcessorIndex processorIndex = InvalidProcessor;
	if (inProcessorIndex != AnyProcessor)
	{
		if (inProcessorIndex >= processorGroup[static_cast<size_t>(processorType)].size())
		{
			return false;
		}

		processorIndex = inProcessorIndex;
	}
	else
	{
		ProcessorIndex leastBusyProcessorIndex = GetLeastBusyProcessorIndex(processorType);
		if (leastBusyProcessorIndex == InvalidProcessor)
		{
			return false;
		}

		processorIndex = leastBusyProcessorIndex;
	}
	processorGroup[static_cast<size_t>(processorType)][processorIndex]->PushTaskToProcessor(std::move(task));

	return true;
}

ProcessorIndex ProcessorManager::GetLeastBusyProcessorIndex(const EProcessorType processorType) const
{
	if (processorType >= EProcessorType::Max)
	{
		return InvalidProcessor;
	}

	std::pair<ProcessorIndex, size_t> leastBusyProcessor{ InvalidProcessor, std::numeric_limits<size_t>::max() };
	for (ProcessorIndex i = 0; i < processorGroup[static_cast<size_t>(processorType)].size(); ++i)
	{
		const auto& processor = processorGroup[static_cast<size_t>(processorType)][i];
		const size_t taskQueueSize = processor->GetTaskQueueSize();
		if (taskQueueSize == 0)
		{
			return i;
		}

		if (taskQueueSize < leastBusyProcessor.second)
		{
			leastBusyProcessor.first = i;
			leastBusyProcessor.second = taskQueueSize;
		}
	}

	return leastBusyProcessor.first;
}
