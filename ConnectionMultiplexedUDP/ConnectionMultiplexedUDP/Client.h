#pragma once

#include <cstdint>

using ClientId = uint64_t;
inline constexpr ClientId InvalidClientId = 0;

class ClientManager;

class Client
{
public:
	Client() = default;
	virtual ~Client() = default;

public:
	ClientId GetClientId() const;
	bool SendPacket();

protected:
	virtual void OnClientCreated() noexcept = 0;
	virtual void OnClientDestroyed() noexcept = 0;
	virtual void OnRecvPacket() = 0;

private:
	friend class ClientManager;

	ClientId clientId = InvalidClientId;
};
