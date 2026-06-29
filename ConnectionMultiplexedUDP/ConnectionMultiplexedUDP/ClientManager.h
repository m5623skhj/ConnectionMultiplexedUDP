#pragma once

#include "Client.h"
#include <cstddef>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <unordered_map>

class ClientPacketSender;

class ClientManager
{
public:
	using ClientVisitor = std::function<void(Client&)>;

private:
	struct ClientEntry
	{
		std::mutex mutex;
		std::condition_variable condition;
		std::unique_ptr<Client> client;
		std::thread::id visitingThreadId;
		bool visitActive = false;
		bool removing = false;
	};

	using ClientEntryPtr = std::shared_ptr<ClientEntry>;

public:
	ClientManager() = default;
	explicit ClientManager(ClientPacketSender& inPacketSender);
	~ClientManager();

	ClientManager(const ClientManager&) = delete;
	ClientManager& operator=(const ClientManager&) = delete;

public:
	ClientId AddClient(std::unique_ptr<Client> inClient);
	// Removal becomes visible immediately. Destruction waits for an in-flight visit to finish.
	bool RemoveClient(ClientId clientId);
	void RemoveAllClients();

	// Visits for a client are serialized. Nested visits from a visitor are rejected.
	bool VisitClient(ClientId clientId, const ClientVisitor& visitor) const;
	bool DispatchPacket(ClientId clientId, PacketType inPacketType, std::string_view inPayload) const;
	bool ContainsClient(ClientId clientId) const;
	size_t GetClientCount() const;

private:
	ClientId AllocateClientId();
	static std::unique_ptr<Client> MarkClientForRemoval(const ClientEntryPtr& clientEntry);
	static std::unique_ptr<Client> FinishClientVisit(const ClientEntryPtr& clientEntry);
	static void DestroyClient(std::unique_ptr<Client> client);

	mutable std::mutex clientsMutex;
	std::unordered_map<ClientId, ClientEntryPtr> clients;
	ClientId nextClientId = 1;
	ClientPacketSender* packetSender = nullptr;
};
