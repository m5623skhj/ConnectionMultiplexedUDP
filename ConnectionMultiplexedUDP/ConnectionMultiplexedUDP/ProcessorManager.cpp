#include "ProcessorManager.h"
#include "IOProcessor.h"
#include "LogicProcessor.h"
#include "TickerProcessor.h"
#include "HeartbeatProcessor.h"
#include <limits>

ProcessorManager::ProcessorManager(
	const int inIoProcessorCount,
	const int inLogicProcessorCount,
	const uint16_t inIoProcessorPortBase,
	const int inTickerProcessorIntervalMs)
	: ioProcessorCount(inIoProcessorCount)
	, logicProcessorCount(inLogicProcessorCount)
	, ioProcessorPortBase(inIoProcessorPortBase)
	, tickerProcessorIntervalMs(inTickerProcessorIntervalMs)
{
	const uint32_t availablePortCount =
		static_cast<uint32_t>((std::numeric_limits<uint16_t>::max)())
		- static_cast<uint32_t>(ioProcessorPortBase) + 1;
	configurationValid = ioProcessorCount > 0
		&& logicProcessorCount > 0
		&& tickerProcessorIntervalMs > 0
		&& static_cast<uint32_t>(ioProcessorCount) <= availablePortCount;
	if (configurationValid)
	{
		RegisterAllProcessor();
	}
}

ProcessorManager::~ProcessorManager()
{
	Stop();
}

bool ProcessorManager::Start()
{
	{
		std::scoped_lock lifecycleLock(lifecycleMutex);
		if (state.load() != EState::Stopped || not configurationValid)
		{
			return false;
		}

		state.store(EState::Starting);
	}

	constexpr std::array startOrder{
		EProcessorType::Logic,
		EProcessorType::TICKER,
		EProcessorType::HEART_BAET,
		EProcessorType::IO };

	try
	{
		for (const EProcessorType processorType : startOrder)
		{
			for (auto& processor : processorGroup[static_cast<size_t>(processorType)])
			{
				if (not processor->Start())
				{
					state.store(EState::Stopping);
					StopAllProcessors();
					state.store(EState::Stopped);
					return false;
				}
			}
		}
	}
	catch (...)
	{
		state.store(EState::Stopping);
		StopAllProcessors();
		state.store(EState::Stopped);
		return false;
	}

	state.store(EState::Running);
	return true;
}

bool ProcessorManager::Stop()
{
	{
		std::scoped_lock lifecycleLock(lifecycleMutex);
		const EState currentState = state.load();
		if (currentState == EState::Stopped)
		{
			return true;
		}
		if (currentState != EState::Running)
		{
			return false;
		}

		for (const auto& processors : processorGroup)
		{
			for (const auto& processor : processors)
			{
				if (processor->IsCurrentThread())
				{
					return false;
				}
			}
		}

		state.store(EState::Stopping);
	}

	StopAllProcessors();
	state.store(EState::Stopped);
	return true;
}

bool ProcessorManager::PushTaskToProcessor(const EProcessorType processorType, std::unique_ptr<ProcessorTaskBase>&& task, const ProcessorIndex inProcessorIndex)
{
	const EState currentState = state.load();
	if ((currentState != EState::Starting && currentState != EState::Running)
		|| processorType >= EProcessorType::Max
		|| task == nullptr)
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
	return processorGroup[static_cast<size_t>(processorType)][processorIndex]->PushTaskToProcessor(std::move(task));
}

bool ProcessorManager::PushTaskToProcessorByAffinity(
	const EProcessorType processorType,
	std::unique_ptr<ProcessorTaskBase>&& task,
	const uint64_t affinityKey)
{
	if (processorType >= EProcessorType::Max)
	{
		return false;
	}

	const auto& processors = processorGroup[static_cast<size_t>(processorType)];
	if (processors.empty())
	{
		return false;
	}

	const ProcessorIndex processorIndex = static_cast<ProcessorIndex>(affinityKey % processors.size());
	return PushTaskToProcessor(processorType, std::move(task), processorIndex);
}

void ProcessorManager::RegisterAllProcessor()
{
	processorGroup[static_cast<size_t>(EProcessorType::IO)].reserve(ioProcessorCount);
	processorGroup[static_cast<size_t>(EProcessorType::Logic)].reserve(logicProcessorCount);

	for (int i = 0; i < ioProcessorCount; ++i)
	{
		processorGroup[static_cast<size_t>(EProcessorType::IO)].push_back(
			std::make_unique<IOProcessor>(*this, static_cast<ProcessorIndex>(i), static_cast<uint16_t>(ioProcessorPortBase + i)));
	}

	for (int i = 0; i < logicProcessorCount; ++i)
	{
		processorGroup[static_cast<size_t>(EProcessorType::Logic)].push_back(std::make_unique<LogicProcessor>(*this));
	}

	processorGroup[static_cast<size_t>(EProcessorType::TICKER)].push_back(std::make_unique<TickerProcessor>(*this, tickerProcessorIntervalMs));
	processorGroup[static_cast<size_t>(EProcessorType::HEART_BAET)].push_back(std::make_unique<HeartbeatProcessor>(*this));
}

void ProcessorManager::StopAllProcessors()
{
	constexpr std::array stopOrder{
		EProcessorType::IO,
		EProcessorType::TICKER,
		EProcessorType::HEART_BAET,
		EProcessorType::Logic };

	for (const EProcessorType processorType : stopOrder)
	{
		for (auto& processor : processorGroup[static_cast<size_t>(processorType)])
		{
			processor->Stop();
		}
	}
}

ProcessorIndex ProcessorManager::GetLeastBusyProcessorIndex(const EProcessorType processorType) const
{
	if (processorType >= EProcessorType::Max)
	{
		return InvalidProcessor;
	}

	std::pair<ProcessorIndex, size_t> leastBusyProcessor{
		InvalidProcessor,
		(std::numeric_limits<size_t>::max)() };
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
