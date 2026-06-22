#pragma once
#include "ProcessorManager.h"
#include <cstdint>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

class ServerCore
{
public:
	ServerCore(
		const int inIoProcessorCount, 
		const int inLogicProcessorCount, 
		const uint16_t ioProcessorPortBase,
		const int inTickMillisecond
	);
	~ServerCore();

public:
	bool Start();
	bool Stop();

private:
	enum class EState : uint8_t
	{
		Stopped,
		Starting,
		Running,
		Stopping,
	};

	std::mutex lifecycleMutex;
	ProcessorManager processorManager;
	EState state = EState::Stopped;
};
