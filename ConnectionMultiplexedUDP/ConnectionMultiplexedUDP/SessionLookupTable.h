#pragma once

#include <cstdint>
#include <queue>
#include <vector>

using ConnectionId = uint64_t;
inline constexpr ConnectionId InvalidConnectionId = 0;

class Session;

class SessionLookupTable
{
public:
	SessionLookupTable() = delete;
	explicit SessionLookupTable(uint32_t maxSessionCount);
	~SessionLookupTable();

public:
	ConnectionId Allocate(Session* session);
	void Release(ConnectionId connectionId);

	Session* Find(ConnectionId connectionId) const;

	static uint32_t GetIndex(ConnectionId connectionId);
	static uint32_t GetGeneration(ConnectionId connectionId);

private:
	static ConnectionId MakeConnectionId(uint32_t index, uint32_t generation);

private:
	struct SessionSlot
	{
		Session* sessionPtr = nullptr;
		uint32_t generation = 0;
	};

	std::vector<SessionSlot> slots;
	std::queue<uint32_t> freeIndices;
};