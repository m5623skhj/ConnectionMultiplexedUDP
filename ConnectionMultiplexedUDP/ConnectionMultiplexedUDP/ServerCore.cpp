#include "ServerCore.h"
#include <winsock2.h>
#include <iostream>
#include <utility>

ServerCore::ServerCore(
	const int inIoProcessorCount, 
	const int inLogicProcessorCount, 
	const uint16_t ioProcessorPortBase,
	const int inTickMillisecond
)
	: clientManager()
	, processorManager(
		clientManager,
		inIoProcessorCount, 
		inLogicProcessorCount, 
		ioProcessorPortBase,
		inTickMillisecond
	)
{
}

ServerCore::~ServerCore()
{
	Stop();
}

bool ServerCore::Start()
{
	{
		std::scoped_lock lock(lifecycleMutex);
		if (state != EState::Stopped)
		{
			return false;
		}
		state = EState::Starting;
	}

	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
		std::scoped_lock lock(lifecycleMutex);
		state = EState::Stopped;
		return false;
	}

	if (not processorManager.Start())
	{
		std::cout << "ProcessorManager failed to start." << std::endl;
		WSACleanup();
		std::scoped_lock lock(lifecycleMutex);
		state = EState::Stopped;
		return false;
	}

	{
		std::scoped_lock lock(lifecycleMutex);
		state = EState::Running;
	}
	return true;
}

bool ServerCore::Stop()
{
	{
		std::scoped_lock lock(lifecycleMutex);
		if (state == EState::Stopped)
		{
			return true;
		}
		if (state != EState::Running)
		{
			return false;
		}
		state = EState::Stopping;
	}

	if (not processorManager.Stop())
	{
		std::scoped_lock lock(lifecycleMutex);
		state = EState::Running;
		return false;
	}
	WSACleanup();

	{
		std::scoped_lock lock(lifecycleMutex);
		state = EState::Stopped;
	}
	return true;
}

ClientId ServerCore::AddClient(std::unique_ptr<Client> inClient)
{
	std::scoped_lock lock(clientSessionMutex);
	return clientManager.AddClient(std::move(inClient));
}

bool ServerCore::RemoveClient(const ClientId clientId)
{
	std::scoped_lock lock(clientSessionMutex);
	processorManager.RemoveClientSessions(clientId);
	return clientManager.RemoveClient(clientId);
}

ConnectionId ServerCore::RegisterClientSession(
	const ClientId clientId,
	const sockaddr_in& inRemoteAddress,
	const cmudp::protocol::AuthenticationKey& inAuthenticationKey)
{
	std::scoped_lock lock(clientSessionMutex);
	if (not clientManager.ContainsClient(clientId))
	{
		return InvalidConnectionId;
	}

	return processorManager.RegisterClientSession(
		clientId,
		inRemoteAddress,
		inAuthenticationKey);
}

bool ServerCore::RemoveClientSession(const ConnectionId connectionId)
{
	std::scoped_lock lock(clientSessionMutex);
	return processorManager.RemoveClientSession(connectionId);
}
