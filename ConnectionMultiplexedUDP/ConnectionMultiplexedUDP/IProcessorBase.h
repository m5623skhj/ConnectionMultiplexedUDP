#pragma once
#include <thread>
#include <functional>

class IProcessorBase
{
public:
	IProcessorBase() = default;
	virtual ~IProcessorBase() = default;

public:
	virtual bool Start() = 0;
	virtual void Stop();

protected:
	std::jthread processorThread;
};