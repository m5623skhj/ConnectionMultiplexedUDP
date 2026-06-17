#pragma once
#include "IProcessorBase.h"
#include "SessionLookupTable.h"
#include <atomic>
#include <winsock2.h>

class IOProcessor : public IProcessorBase
{
public:
	IOProcessor();
	~IOProcessor();

public:
	bool Start() override;
	void Stop() override;

public:
	bool SendPacket(const sockaddr_in& destAddr, const char* data, int dataSize);

private:
	void RunRecvThread();

private:
	SessionLookupTable sessionLookupTable;
	
	SOCKET recvSocket;
	SOCKET sendSocket;
	std::atomic_bool needStop = false;
};