#pragma once

class IProcessorBase
{
public:
	IProcessorBase() = default;
	virtual ~IProcessorBase() = default;

public:
	virtual bool Start() = 0;
	virtual void Stop() = 0;
};