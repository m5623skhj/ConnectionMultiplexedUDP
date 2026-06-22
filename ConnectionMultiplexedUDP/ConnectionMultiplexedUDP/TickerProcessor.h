#pragma once
#pragma once
#include "ProcessorBase.h"

class TickerProcessor : public ProcessorBase
{
public:
	TickerProcessor() = delete;
	explicit TickerProcessor(ProcessorManager& inProcessorManager, const int inTickMillisecond);
	~TickerProcessor();

public:
	bool StartImpl() override;
	void StopImpl() override;

private:
	void RunTickerThread();
	void ProcessTask(std::unique_ptr<ProcessorTask>&& task) override;

private:
	int tickMillisecond;
};