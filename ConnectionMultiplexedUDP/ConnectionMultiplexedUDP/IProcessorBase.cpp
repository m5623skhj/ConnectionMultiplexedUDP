#include "IProcessorBase.h"

void IProcessorBase::Stop()
{
	if (processorThread.joinable())
	{
		processorThread.request_stop();
		processorThread.join();
	}
}