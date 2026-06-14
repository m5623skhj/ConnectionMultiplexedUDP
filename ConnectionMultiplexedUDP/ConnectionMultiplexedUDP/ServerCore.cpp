#include "ServerCore.h"
#include <winsock2.h>
#include <iostream>

ServerCore::ServerCore(int inIoProcessorCount, int inLogicProcessorCount)
	: processorManager(inIoProcessorCount, inLogicProcessorCount)
{
}

ServerCore::~ServerCore()
{
	Stop();
}

bool ServerCore::Start()
{
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
		return false;
	}

	return true;

	if (not processorManager.Start())
	{
		std::cout << "ProcessorManager failed to start." << std::endl;
		return false;
	}

	return true;
}

void ServerCore::Stop()
{
}
