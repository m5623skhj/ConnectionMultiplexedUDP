#include "SmokeTest.h"

#include "Generated/PacketEnvelope.pb.h"
#include "Protocol/PacketAuthentication.h"
#include "ServerCore.h"
#include <array>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <winsock2.h>

namespace
{
	constexpr uint16_t SMOKE_TEST_SERVER_PORT = 37777;
	constexpr PacketType CLIENT_TO_SERVER_PACKET_TYPE = cmudp::protocol::FIRST_APPLICATION_PACKET_TYPE + 1;
	constexpr PacketType SERVER_TO_CLIENT_PACKET_TYPE = cmudp::protocol::FIRST_APPLICATION_PACKET_TYPE + 2;
	constexpr std::chrono::milliseconds PACKET_WAIT_TIMEOUT{ 2000 };
	constexpr int SMOKE_TEST_SESSION_TIMEOUT_MS = 500;

	class SmokeTestClient final : public Client
	{
	public:
		bool WaitForPacket(
			const PacketType inPacketType,
			const std::string_view inPayload,
			const std::chrono::milliseconds inTimeout)
		{
			std::unique_lock lock(mutex);
			return condition.wait_for(lock, inTimeout, [this, inPacketType, inPayload]
				{
					return receivedPacketType == inPacketType && receivedPayload == inPayload;
				});
		}

	protected:
		void OnClientCreated() noexcept override
		{
		}

		void OnClientDestroyed() noexcept override
		{
		}

		void OnRecvPacket(const PacketType inPacketType, const std::string_view inPayload) override
		{
			{
				std::scoped_lock lock(mutex);
				receivedPacketType = inPacketType;
				receivedPayload.assign(inPayload.data(), inPayload.size());
			}
			condition.notify_all();
		}

	private:
		std::mutex mutex;
		std::condition_variable condition;
		PacketType receivedPacketType = INVALID_PACKET_TYPE;
		std::string receivedPayload;
	};

	class SocketHandle final
	{
	public:
		SocketHandle() = default;
		explicit SocketHandle(const SOCKET inSocket)
			: socket(inSocket)
		{
		}

		~SocketHandle()
		{
			Reset();
		}

		SocketHandle(const SocketHandle&) = delete;
		SocketHandle& operator=(const SocketHandle&) = delete;

		SocketHandle(SocketHandle&& other) noexcept
			: socket(other.socket)
		{
			other.socket = INVALID_SOCKET;
		}

		SocketHandle& operator=(SocketHandle&& other) noexcept
		{
			if (this != &other)
			{
				Reset();
				socket = other.socket;
				other.socket = INVALID_SOCKET;
			}
			return *this;
		}

		SOCKET Get() const
		{
			return socket;
		}

		SOCKET Release()
		{
			const SOCKET releasedSocket = socket;
			socket = INVALID_SOCKET;
			return releasedSocket;
		}

		void Reset()
		{
			if (socket != INVALID_SOCKET)
			{
				closesocket(socket);
				socket = INVALID_SOCKET;
			}
		}

	private:
		SOCKET socket = INVALID_SOCKET;
	};

	bool BindLoopbackClientSocket(SocketHandle& outSocket, sockaddr_in& outLocalAddress)
	{
		SocketHandle clientSocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
		if (clientSocket.Get() == INVALID_SOCKET)
		{
			std::cout << "client socket failed: " << WSAGetLastError() << '\n';
			return false;
		}

		constexpr DWORD RECEIVE_TIMEOUT_MS = 2000;
		if (setsockopt(
			clientSocket.Get(),
			SOL_SOCKET,
			SO_RCVTIMEO,
			reinterpret_cast<const char*>(&RECEIVE_TIMEOUT_MS),
			sizeof(RECEIVE_TIMEOUT_MS)) == SOCKET_ERROR)
		{
			std::cout << "client setsockopt failed: " << WSAGetLastError() << '\n';
			return false;
		}

		sockaddr_in bindAddress{};
		bindAddress.sin_family = AF_INET;
		bindAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		bindAddress.sin_port = 0;
		if (bind(
			clientSocket.Get(),
			reinterpret_cast<const sockaddr*>(&bindAddress),
			sizeof(bindAddress)) == SOCKET_ERROR)
		{
			std::cout << "client bind failed: " << WSAGetLastError() << '\n';
			return false;
		}

		int addressLength = sizeof(outLocalAddress);
		if (getsockname(
			clientSocket.Get(),
			reinterpret_cast<sockaddr*>(&outLocalAddress),
			&addressLength) == SOCKET_ERROR)
		{
			std::cout << "client getsockname failed: " << WSAGetLastError() << '\n';
			return false;
		}

		outSocket = SocketHandle(clientSocket.Release());
		return true;
	}

	bool SendPacketToServer(
		const SOCKET clientSocket,
		const sockaddr_in& serverAddress,
		const ConnectionId connectionId,
		const uint64_t sequence,
		const PacketType packetType,
		const std::string_view payload,
		const cmudp::protocol::AuthenticationKey& authenticationKey)
	{
		std::string packetData;
		if (not cmudp::protocol::SerializeAuthenticatedPacket(
				connectionId,
				sequence,
				packetType,
				payload,
				authenticationKey,
				packetData))
		{
			std::cout << "failed to serialize client packet\n";
			return false;
		}

		const int sentSize = sendto(
			clientSocket,
			packetData.data(),
			static_cast<int>(packetData.size()),
			0,
			reinterpret_cast<const sockaddr*>(&serverAddress),
			sizeof(serverAddress));
		if (sentSize == SOCKET_ERROR)
		{
			std::cout << "client sendto failed: " << WSAGetLastError() << '\n';
			return false;
		}

		return true;
	}

	bool WaitUntil(
		const std::function<bool()>& predicate,
		const std::chrono::milliseconds timeout)
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (std::chrono::steady_clock::now() < deadline)
		{
			if (predicate())
			{
				return true;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}

		return predicate();
	}

	bool ReceiveServerPacket(
		const SOCKET clientSocket,
		const ConnectionId connectionId,
		const cmudp::protocol::AuthenticationKey& authenticationKey)
	{
		std::array<char, cmudp::protocol::MAX_PACKET_SIZE> recvBuffer{};
		sockaddr_in senderAddress{};
		int senderAddressLength = sizeof(senderAddress);
		const int receivedSize = recvfrom(
			clientSocket,
			recvBuffer.data(),
			static_cast<int>(recvBuffer.size()),
			0,
			reinterpret_cast<sockaddr*>(&senderAddress),
			&senderAddressLength);
		if (receivedSize == SOCKET_ERROR)
		{
			std::cout << "client recvfrom failed: " << WSAGetLastError() << '\n';
			return false;
		}

		cmudp::protocol::PacketEnvelope envelope;
		if (not envelope.ParseFromArray(recvBuffer.data(), receivedSize)
			|| envelope.connection_id() != connectionId
			|| not cmudp::protocol::VerifyAuthenticationTag(
				authenticationKey,
				envelope.authenticated_data(),
				envelope.authentication_tag()))
		{
			std::cout << "server packet envelope validation failed\n";
			return false;
		}

		cmudp::protocol::AuthenticatedPacket authenticatedPacket;
		if (not authenticatedPacket.ParseFromString(envelope.authenticated_data())
			|| authenticatedPacket.connection_id() != connectionId
			|| authenticatedPacket.packet_type() != SERVER_TO_CLIENT_PACKET_TYPE
			|| authenticatedPacket.payload() != "server-to-client")
		{
			std::cout << "server packet payload validation failed\n";
			return false;
		}

		return true;
	}
}

int RunSmokeTest()
{
	ServerCore serverCore(
		1,
		1,
		SMOKE_TEST_SERVER_PORT,
		100,
		SMOKE_TEST_SESSION_TIMEOUT_MS);
	if (not serverCore.Start())
	{
		std::cout << "smoke test failed: server failed to start\n";
		return 1;
	}

	SocketHandle clientSocket;
	sockaddr_in clientAddress{};
	if (not BindLoopbackClientSocket(clientSocket, clientAddress))
	{
		serverCore.Stop();
		return 1;
	}

	auto client = std::make_unique<SmokeTestClient>();
	SmokeTestClient* clientPtr = client.get();
	const ClientId clientId = serverCore.AddClient(std::move(client));
	if (clientId == InvalidClientId)
	{
		std::cout << "smoke test failed: AddClient failed\n";
		serverCore.Stop();
		return 1;
	}

	cmudp::protocol::AuthenticationKey authenticationKey{};
	if (not cmudp::protocol::GenerateAuthenticationKey(authenticationKey))
	{
		std::cout << "smoke test failed: authentication key generation failed\n";
		serverCore.Stop();
		return 1;
	}

	const ConnectionId connectionId = serverCore.RegisterClientSession(
		clientId,
		clientAddress,
		authenticationKey);
	if (connectionId == InvalidConnectionId)
	{
		std::cout << "smoke test failed: RegisterClientSession failed\n";
		serverCore.Stop();
		return 1;
	}

	sockaddr_in serverAddress{};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	serverAddress.sin_port = htons(SMOKE_TEST_SERVER_PORT);
	if (not SendPacketToServer(
			clientSocket.Get(),
			serverAddress,
			connectionId,
			1,
			CLIENT_TO_SERVER_PACKET_TYPE,
			"client-to-server",
			authenticationKey)
		|| not clientPtr->WaitForPacket(
			CLIENT_TO_SERVER_PACKET_TYPE,
			"client-to-server",
			PACKET_WAIT_TIMEOUT))
	{
		std::cout << "smoke test failed: receive path failed\n";
		serverCore.Stop();
		return 1;
	}

	if (not clientPtr->SendPacket(
			connectionId,
			SERVER_TO_CLIENT_PACKET_TYPE,
			"server-to-client")
		|| not ReceiveServerPacket(
			clientSocket.Get(),
			connectionId,
			authenticationKey))
	{
		std::cout << "smoke test failed: send path failed\n";
		serverCore.Stop();
		return 1;
	}

	const ConnectionId disconnectConnectionId = serverCore.RegisterClientSession(
		clientId,
		clientAddress,
		authenticationKey);
	if (disconnectConnectionId == InvalidConnectionId
		|| not SendPacketToServer(
			clientSocket.Get(),
			serverAddress,
			disconnectConnectionId,
			1,
			cmudp::protocol::DISCONNECT_PACKET_TYPE,
			"",
			authenticationKey)
		|| not WaitUntil(
			[clientPtr, disconnectConnectionId]
			{
				return not clientPtr->SendPacket(
					disconnectConnectionId,
					SERVER_TO_CLIENT_PACKET_TYPE,
					"after-disconnect");
			},
			PACKET_WAIT_TIMEOUT))
	{
		std::cout << "smoke test failed: disconnect path failed\n";
		serverCore.Stop();
		return 1;
	}

	const ConnectionId timeoutConnectionId = serverCore.RegisterClientSession(
		clientId,
		clientAddress,
		authenticationKey);
	if (timeoutConnectionId == InvalidConnectionId
		|| not WaitUntil(
			[clientPtr, timeoutConnectionId]
			{
				return not clientPtr->SendPacket(
					timeoutConnectionId,
					SERVER_TO_CLIENT_PACKET_TYPE,
					"after-timeout");
			},
			PACKET_WAIT_TIMEOUT))
	{
		std::cout << "smoke test failed: timeout disconnect failed\n";
		serverCore.Stop();
		return 1;
	}

	serverCore.Stop();
	std::cout << "smoke test passed\n";
	return 0;
}
