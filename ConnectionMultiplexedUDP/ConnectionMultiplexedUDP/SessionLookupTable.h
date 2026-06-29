#pragma once

#include "Client.h"
#include "ConnectionId.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

class Session;

class SessionLookupTable
{
public:
	using SessionSnapshot = std::vector<std::pair<ConnectionId, std::shared_ptr<Session>>>;

public:
	SessionLookupTable() = delete;
	explicit SessionLookupTable(uint32_t maxSessionCount);
	~SessionLookupTable();

public:
	ConnectionId Allocate(std::shared_ptr<Session> inSession);
	bool Release(ConnectionId connectionId);
	size_t ReleaseByClientId(ClientId clientId);

	std::shared_ptr<Session> Find(ConnectionId connectionId) const;
	SessionSnapshot SnapshotActiveSessions() const;

	static uint32_t GetIndex(ConnectionId connectionId);
	static uint32_t GetGeneration(ConnectionId connectionId);

private:
	static ConnectionId MakeConnectionId(uint32_t index, uint32_t generation);

private:
	struct SessionSlot
	{
		std::shared_ptr<Session> session;
		uint32_t generation = 1;
	};

	mutable std::mutex slotsMutex;
	std::vector<SessionSlot> slots;
	std::queue<uint32_t> freeIndices;
};
