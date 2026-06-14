#pragma once
#include "IProcessorBase.h"
#include <array>
#include <vector>

enum class EProcessorType : uint8_t
{
	IO,
	Logic,
	
	Max,
};

class ProcessorManager
{
public:
	ProcessorManager() = delete;
	explicit ProcessorManager(int inIoProcessorCount, int inLogicProcessorCount);
	~ProcessorManager();

public:
	bool Start();
	bool Stop();

private:
	int ioProcessorCount;
	int logicProcessorCount;
	
private:
	std::array<std::vector<IProcessorBase*>, static_cast<size_t>(EProcessorType::Max)> processorGroup;
};