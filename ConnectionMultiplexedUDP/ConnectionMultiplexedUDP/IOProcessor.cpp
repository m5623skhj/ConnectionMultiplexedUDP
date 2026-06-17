#include "IOProcessor.h"

IOProcessor::IOProcessor()
	: sessionLookupTable(1000)
{
}

IOProcessor::~IOProcessor()
{
}

bool IOProcessor::Start()
{
	processorThread = std::jthread(&IOProcessor::RunIOThread, this);

	return true;
}

void IOProcessor::Stop()
{
}

void IOProcessor::RunIOThread()
{
}
