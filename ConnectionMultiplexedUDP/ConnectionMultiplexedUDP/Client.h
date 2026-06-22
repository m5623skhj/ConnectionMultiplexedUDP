#pragma once

class Client
{
public:
	Client() = default;
	virtual ~Client() = default;

public:
	bool SendPacket();

protected:
	virtual void OnClientCreated() = 0;
	virtual void OnClientDestroyed() = 0;
	virtual void OnRecvPacket() = 0;

private:

};