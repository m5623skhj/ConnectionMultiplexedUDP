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
	for (auto& processorGroup : processorGroup)
	{
		for (auto& processor : processorGroup)
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
	for (auto& processorGroup : processorGroup)
	{
		for (auto& processor : processorGroup)
		{
			processor->Stop();
		}
	}
}
