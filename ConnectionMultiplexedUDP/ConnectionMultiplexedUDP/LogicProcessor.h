#pragma once
#include "ProcessorBase.h"

class LogicProcessor : public ProcessorBase
{
public:
	LogicProcessor();
	~LogicProcessor();

public:
	bool Start() override;
	void Stop() override;

private:
	void RunLogicThread();
	void ProcessTask(std::unique_ptr<ProcessorTask>&& task) override;

private:
};