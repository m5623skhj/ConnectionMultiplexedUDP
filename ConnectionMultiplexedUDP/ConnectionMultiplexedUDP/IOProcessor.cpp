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
	return true;
}

void IOProcessor::Stop()
{
}
