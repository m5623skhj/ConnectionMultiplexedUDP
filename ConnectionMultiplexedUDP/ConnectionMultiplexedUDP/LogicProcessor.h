#pragma once
#include "ProcessorBase.h"

class ClientDisconnectTask;
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
	void ProcessClientDisconnect(const ClientDisconnectTask& task);
	void ProcessReceivedPacket(const ReceivedPacketTask& task);
};
