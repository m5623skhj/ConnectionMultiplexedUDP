#pragma once
#include "ProcessorBase.h"
#include "ProcessorType.h"
#include "SessionLookupTable.h"
#include <mutex>
#include <winsock2.h>

class IOProcessor : public ProcessorBase
{
public:
	IOProcessor() = delete;
	explicit IOProcessor(
		ProcessorManager& inProcessorManager,
		ProcessorIndex inProcessorIndex,
		uint16_t inPort);
	~IOProcessor() override;

public:
	bool StartImpl() override;
	void StopImpl() override;
	void StopAfterProcessorThreadImpl() override;

public:
	bool SendPacket(const sockaddr_in& destAddr, const char* data, int dataSize);

private:
	void RunRecvThread();
	void ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task) override;

private:
	SessionLookupTable sessionLookupTable;
	ProcessorIndex processorIndex;

	mutable std::mutex socketMutex;
	SOCKET sock;
	short port;
};
