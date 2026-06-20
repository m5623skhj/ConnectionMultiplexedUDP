#pragma once
#include "ProcessorManager.h"

#pragma comment(lib, "ws2_32.lib")

class ServerCore
{
public:
	ServerCore(const int inIoProcessorCount, const int inLogicProcessorCount, const uint16_t ioProcessorPortBase);
	~ServerCore();

public:
	bool Start();
	void Stop();

private:
	ProcessorManager processorManager;
	bool started = false;
};