#pragma once
#include "IProcessorBase.h"

class LogicProcessor : public IProcessorBase
{
public:
	LogicProcessor();
	~LogicProcessor();

public:
	bool Start() override;
	void Stop() override;

private:
};