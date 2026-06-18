#pragma once
#include "ProcessorBase.h"
#include <array>
#include <vector>
#include "ProcessorType.h"

class ProcessorManager
{
public:
	ProcessorManager() = delete;
	explicit ProcessorManager(int inIoProcessorCount, int inLogicProcessorCount);
	~ProcessorManager();

public:
	bool Start();
	void Stop();

private:
	int ioProcessorCount;
	int logicProcessorCount;
	
private:
	std::array<std::vector<std::unique_ptr<ProcessorBase>>, static_cast<size_t>(EProcessorType::Max)> processorGroup;
};