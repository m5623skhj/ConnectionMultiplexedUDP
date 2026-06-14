#pragma once

#include "ProcessorManager.h"

#pragma comment(lib, "ws2_32.lib")

class ServerCore
{
public:
	ServerCore(int inIoProcessorCount, int inLogicProcessorCount);
	~ServerCore();

public:
	bool Start();
	void Stop();

private:
	ProcessorManager processorManager;
};