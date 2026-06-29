#include "SessionLookupTable.h"
#include "Session.h"

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

ConnectionId SessionLookupTable::Allocate(std::shared_ptr<Session> inSession)
{
	if (inSession == nullptr)
	{
		return InvalidConnectionId;
	}

	std::scoped_lock lock(slotsMutex);
	if (freeIndices.empty())
	{
		return InvalidConnectionId;
	}

	const uint32_t index = freeIndices.front();
	freeIndices.pop();

	SessionSlot& slot = slots[index];
	slot.session = std::move(inSession);

	return MakeConnectionId(index, slot.generation);
}

bool SessionLookupTable::Release(ConnectionId connectionId)
{
	std::shared_ptr<Session> removedSession;
	{
		std::scoped_lock lock(slotsMutex);
		const uint32_t index = GetIndex(connectionId);
		if (index >= slots.size())
		{
			return false;
		}

		SessionSlot& slot = slots[index];
		if (slot.session == nullptr || slot.generation != GetGeneration(connectionId))
		{
			return false;
		}

		removedSession = std::move(slot.session);
		++slot.generation;
		if (slot.generation == 0)
		{
			++slot.generation;
		}

		freeIndices.push(index);
	}

	if (removedSession == nullptr)
	{
		return false;
	}
	return true;
}

size_t SessionLookupTable::ReleaseByClientId(const ClientId clientId)
{
	std::vector<std::shared_ptr<Session>> removedSessions;
	{
		std::scoped_lock lock(slotsMutex);
		for (uint32_t index = 0; index < slots.size(); ++index)
		{
			SessionSlot& slot = slots[index];
			if (slot.session == nullptr || slot.session->GetClientId() != clientId)
			{
				continue;
			}

			removedSessions.push_back(std::move(slot.session));
			++slot.generation;
			if (slot.generation == 0)
			{
				++slot.generation;
			}
			freeIndices.push(index);
		}
	}
	return removedSessions.size();
}

std::shared_ptr<Session> SessionLookupTable::Find(ConnectionId connectionId) const
{
	if (connectionId == InvalidConnectionId)
	{
		return nullptr;
	}

	std::scoped_lock lock(slotsMutex);
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

	return slot.session;
}

SessionLookupTable::SessionSnapshot SessionLookupTable::SnapshotActiveSessions() const
{
	SessionSnapshot snapshot;
	std::scoped_lock lock(slotsMutex);
	snapshot.reserve(slots.size() - freeIndices.size());

	for (uint32_t index = 0; index < slots.size(); ++index)
	{
		const SessionSlot& slot = slots[index];
		if (slot.session == nullptr)
		{
			continue;
		}

		snapshot.emplace_back(
			MakeConnectionId(index, slot.generation),
			slot.session);
	}

	return snapshot;
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
