#pragma once
#include "IProcessorBase.h"
#include "SessionLookupTable.h"
#include <winsock2.h>

class IOProcessor : public IProcessorBase
{
public:
	IOProcessor();
	~IOProcessor();

public:
	bool Start() override;
	void Stop() override;

private:
	void RunIOThread();

private:
	SessionLookupTable sessionLookupTable;
	SOCKET socket;
};