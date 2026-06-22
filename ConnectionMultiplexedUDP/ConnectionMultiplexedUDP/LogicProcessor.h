#pragma once
#include "ProcessorBase.h"

class ReceivedPacketTask;

class LogicProcessor : public ProcessorBase
{
public:
	LogicProcessor() = delete;
	explicit LogicProcessor(ProcessorManager& inProcessorManager);
	~LogicProcessor() override;

public:
	bool StartImpl() override;
	void StopImpl() override;

private:
	void ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task) override;
	void ProcessReceivedPacket(const ReceivedPacketTask& task);
};
