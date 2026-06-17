#pragma once

class Client
{
public:
	Client() = default;
	virtual ~Client() = default;

	virtual void OnClientCreated() = 0;
	virtual void OnClientDestroyed() = 0;

private:

};