#include "ClientManager.h"

#include <utility>
#include <vector>

ClientManager::ClientManager(ClientPacketSender& inPacketSender)
	: packetSender(&inPacketSender)
{
}

ClientManager::~ClientManager()
{
	RemoveAllClients();
}

ClientId ClientManager::AddClient(std::unique_ptr<Client> inClient)
{
	if (inClient == nullptr)
	{
		return InvalidClientId;
	}

	ClientId clientId = InvalidClientId;
	{
		std::scoped_lock lock(clientsMutex);

		clientId = AllocateClientId();
		if (clientId == InvalidClientId)
		{
			return InvalidClientId;
		}

		inClient->clientId = clientId;
		inClient->packetSender = packetSender;
	}

	inClient->OnClientCreated();
	auto clientEntry = std::make_shared<ClientEntry>();
	clientEntry->client = std::move(inClient);

	{
		std::scoped_lock lock(clientsMutex);
		clients.emplace(clientId, std::move(clientEntry));
	}

	return clientId;
}

bool ClientManager::RemoveClient(const ClientId clientId)
{
	ClientEntryPtr removedEntry;
	{
		std::scoped_lock lock(clientsMutex);

		const auto clientIter = clients.find(clientId);
		if (clientIter == clients.end())
		{
			return false;
		}

		removedEntry = std::move(clientIter->second);
		clients.erase(clientIter);
	}

	DestroyClient(MarkClientForRemoval(removedEntry));
	return true;
}

void ClientManager::RemoveAllClients()
{
	std::vector<ClientEntryPtr> removedEntries;
	{
		std::scoped_lock lock(clientsMutex);
		removedEntries.reserve(clients.size());

		for (auto& clientEntry : clients)
		{
			removedEntries.push_back(std::move(clientEntry.second));
		}

		clients.clear();
	}

	for (const ClientEntryPtr& clientEntry : removedEntries)
	{
		DestroyClient(MarkClientForRemoval(clientEntry));
	}
}

bool ClientManager::VisitClient(const ClientId clientId, const ClientVisitor& visitor) const
{
	thread_local bool isVisitingClient = false;
	if (not visitor || isVisitingClient)
	{
		return false;
	}

	ClientEntryPtr clientEntry;
	{
		std::scoped_lock lock(clientsMutex);
		const auto clientIter = clients.find(clientId);
		if (clientIter == clients.end())
		{
			return false;
		}

		clientEntry = clientIter->second;
	}

	Client* client = nullptr;
	{
		std::unique_lock entryLock(clientEntry->mutex);
		if (clientEntry->visitActive
			&& clientEntry->visitingThreadId == std::this_thread::get_id())
		{
			return false;
		}

		clientEntry->condition.wait(entryLock, [&clientEntry]
			{
				return not clientEntry->visitActive || clientEntry->removing;
			});

		if (clientEntry->removing || clientEntry->client == nullptr)
		{
			return false;
		}

		clientEntry->visitActive = true;
		clientEntry->visitingThreadId = std::this_thread::get_id();
		client = clientEntry->client.get();
	}

	isVisitingClient = true;
	try
	{
		visitor(*client);
	}
	catch (...)
	{
		isVisitingClient = false;
		DestroyClient(FinishClientVisit(clientEntry));
		throw;
	}

	isVisitingClient = false;
	DestroyClient(FinishClientVisit(clientEntry));
	return true;
}

bool ClientManager::DispatchPacket(
	const ClientId clientId,
	const PacketType inPacketType,
	const std::string_view inPayload) const
{
	if (inPacketType == INVALID_PACKET_TYPE)
	{
		return false;
	}

	return VisitClient(clientId, [inPacketType, inPayload](Client& client)
		{
			client.OnRecvPacket(inPacketType, inPayload);
		});
}

bool ClientManager::ContainsClient(const ClientId clientId) const
{
	std::scoped_lock lock(clientsMutex);
	return clients.contains(clientId);
}

size_t ClientManager::GetClientCount() const
{
	std::scoped_lock lock(clientsMutex);
	return clients.size();
}

ClientId ClientManager::AllocateClientId()
{
	if (nextClientId == InvalidClientId)
	{
		return InvalidClientId;
	}

	return nextClientId++;
}

std::unique_ptr<Client> ClientManager::MarkClientForRemoval(const ClientEntryPtr& clientEntry)
{
	std::scoped_lock entryLock(clientEntry->mutex);
	clientEntry->removing = true;
	clientEntry->condition.notify_all();
	if (clientEntry->visitActive)
	{
		return nullptr;
	}

	return std::move(clientEntry->client);
}

std::unique_ptr<Client> ClientManager::FinishClientVisit(const ClientEntryPtr& clientEntry)
{
	std::scoped_lock entryLock(clientEntry->mutex);
	clientEntry->visitActive = false;
	clientEntry->visitingThreadId = {};
	clientEntry->condition.notify_one();
	if (not clientEntry->removing)
	{
		return nullptr;
	}

	return std::move(clientEntry->client);
}

void ClientManager::DestroyClient(std::unique_ptr<Client> client)
{
	if (client != nullptr)
	{
		client->packetSender = nullptr;
		client->OnClientDestroyed();
	}
}
