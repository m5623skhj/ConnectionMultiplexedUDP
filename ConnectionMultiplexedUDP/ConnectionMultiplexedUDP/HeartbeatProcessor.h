#pragma once
#include "ProcessorBase.h"
#include <chrono>
#include <memory>

class HeartbeatProcessor : public ProcessorBase
{
public:
	HeartbeatProcessor() = delete;
	explicit HeartbeatProcessor(
		ProcessorManager& inProcessorManager,
		std::chrono::milliseconds inSessionTimeout);
	~HeartbeatProcessor() override;

public:
	bool StartImpl() override;
	void StopImpl() override;

private:
	void RunHeartbeatThread();
	void ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task) override;

private:
	std::chrono::milliseconds sessionTimeout;
};
