#pragma once
#include "ProcessorTaskBase.h"
#include "SessionLookupTable.h"

class ClientDisconnectTask final : public ProcessorTaskBase
{
public:
	explicit ClientDisconnectTask(const ConnectionId inConnectionId)
		: connectionId(inConnectionId)
	{
	}
	~ClientDisconnectTask() override = default;

public:
	ConnectionId GetConnectionId() const
	{
		return connectionId;
	}

private:
	ConnectionId connectionId = InvalidConnectionId;
};
