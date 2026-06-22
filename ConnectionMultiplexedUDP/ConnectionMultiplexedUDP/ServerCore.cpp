#include "ServerCore.h"
#include <winsock2.h>
#include <iostream>

ServerCore::ServerCore(
	const int inIoProcessorCount, 
	const int inLogicProcessorCount, 
	const uint16_t ioProcessorPortBase,
	const int inTickMillisecond
)
	: processorManager(
		inIoProcessorCount, 
		inLogicProcessorCount, 
		ioProcessorPortBase,
		inTickMillisecond
	)
{
}

ServerCore::~ServerCore()
{
	Stop();
}

bool ServerCore::Start()
{
	{
		std::scoped_lock lock(lifecycleMutex);
		if (state != EState::Stopped)
		{
			return false;
		}
		state = EState::Starting;
	}

	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
		std::scoped_lock lock(lifecycleMutex);
		state = EState::Stopped;
		return false;
	}

	if (not processorManager.Start())
	{
		std::cout << "ProcessorManager failed to start." << std::endl;
		WSACleanup();
		std::scoped_lock lock(lifecycleMutex);
		state = EState::Stopped;
		return false;
	}

	{
		std::scoped_lock lock(lifecycleMutex);
		state = EState::Running;
	}
	return true;
}

bool ServerCore::Stop()
{
	{
		std::scoped_lock lock(lifecycleMutex);
		if (state == EState::Stopped)
		{
			return true;
		}
		if (state != EState::Running)
		{
			return false;
		}
		state = EState::Stopping;
	}

	if (not processorManager.Stop())
	{
		std::scoped_lock lock(lifecycleMutex);
		state = EState::Running;
		return false;
	}
	WSACleanup();

	{
		std::scoped_lock lock(lifecycleMutex);
		state = EState::Stopped;
	}
	return true;
}
