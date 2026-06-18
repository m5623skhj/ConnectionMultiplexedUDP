#pragma once
#include "ProcessorBase.h"
#include "SessionLookupTable.h"
#include <winsock2.h>

class IOProcessor : public ProcessorBase
{
public:
	IOProcessor();
	~IOProcessor();

public:
	bool StartImpl() override;
	void StopImpl() override;

public:
	bool SendPacket(const sockaddr_in& destAddr, const char* data, int dataSize);

private:
	void RunRecvThread();
	void ProcessTask(std::unique_ptr<ProcessorTask>&& task) override;

private:
	SessionLookupTable sessionLookupTable;
	
	SOCKET recvSocket;
	SOCKET sendSocket;
};