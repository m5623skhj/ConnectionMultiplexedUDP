#pragma once
#include "ProcessorBase.h"

class LogicProcessor : public ProcessorBase
{
public:
	LogicProcessor() = delete;
	explicit LogicProcessor(ProcessorManager& inProcessorManager);
	~LogicProcessor();

public:
	bool StartImpl() override;
	void StopImpl() override;

private:
	void RunLogicThread();
	void ProcessTask(std::unique_ptr<ProcessorTask>&& task) override;

private:
};