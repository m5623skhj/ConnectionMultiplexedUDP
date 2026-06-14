#include "SessionLookupTable.h"

SessionLookupTable::SessionLookupTable(uint32_t maxSessionCount)
	: slots(maxSessionCount)
{
	for (uint32_t i = 0; i < maxSessionCount; ++i)
	{
		freeIndices.push(i);
	}
}

SessionLookupTable::~SessionLookupTable()
{
}

ConnectionId SessionLookupTable::Allocate(Session* session)
{
	const uint32_t index = freeIndices.front();
	freeIndices.pop();

	SessionSlot& slot = slots[index];
	slot.sessionPtr = session;

	return MakeConnectionId(index, slot.generation);
}

void SessionLookupTable::Release(ConnectionId connectionId)
{
	const uint32_t index = GetIndex(connectionId);
	if (index >= slots.size())
	{
		return;
	}

	SessionSlot& slot = slots[index];
	slot.sessionPtr = nullptr;
	++slot.generation;

	freeIndices.push(index);
}

Session* SessionLookupTable::Find(ConnectionId connectionId) const
{
	const uint32_t index = GetIndex(connectionId);
	if (index >= slots.size())
	{
		return nullptr;
	}

	const SessionSlot& slot = slots[index];
	if (slot.generation != GetGeneration(connectionId))
	{
		return nullptr;
	}

	return slot.sessionPtr;
}

uint32_t SessionLookupTable::GetIndex(ConnectionId connectionId)
{
	return static_cast<uint32_t>(connectionId);
}

uint32_t SessionLookupTable::GetGeneration(ConnectionId connectionId)
{
	return static_cast<uint32_t>(connectionId >> 32);
}

ConnectionId SessionLookupTable::MakeConnectionId(uint32_t index, uint32_t generation)
{
	return (static_cast<uint64_t>(generation) << 32) | static_cast<uint64_t>(index);
}