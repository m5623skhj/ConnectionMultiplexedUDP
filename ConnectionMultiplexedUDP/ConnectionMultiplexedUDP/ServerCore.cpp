#include "ServerCore.h"
#include <winsock2.h>
#include <iostream>

ServerCore::ServerCore(
	const int inIoProcessorCount, 
	const int inLogicProcessorCount, 
	const uint16_t ioProcessorPortBase)
	: processorManager(inIoProcessorCount, inLogicProcessorCount, ioProcessorPortBase)
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

	if (not processorManager.Start())
	{
		std::cout << "ProcessorManager failed to start." << std::endl;
		WSACleanup();
		return false;
	}

	started = true;
	return true;
}

void ServerCore::Stop()
{
	if (not started)
	{
		return;
	}

	processorManager.Stop();
	WSACleanup();

	started = false;
}
