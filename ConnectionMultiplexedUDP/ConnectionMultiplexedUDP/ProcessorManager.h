#pragma once
#include "ProcessorBase.h"
#include <array>
#include <vector>
#include "ProcessorType.h"

class ProcessorManager
{
public:
	ProcessorManager() = delete;
	explicit ProcessorManager(
		const int inIoProcessorCount, 
		const int inLogicProcessorCount, 
		const uint16_t ioProcessorPortBase,
		const int inTickMillisecond
	);
	~ProcessorManager();

public:
	bool Start();
	void Stop();

public:
	bool PushTaskToProcessor(const EProcessorType processorType, std::unique_ptr<ProcessorTask>&& task, const ProcessorIndex c = AnyProcessor);

private:
	ProcessorIndex GetLeastBusyProcessorIndex(const EProcessorType processorType) const;

private:
	int ioProcessorCount;
	int logicProcessorCount;
	
private:
	std::array<std::vector<std::unique_ptr<ProcessorBase>>, static_cast<size_t>(EProcessorType::Max)> processorGroup;
};