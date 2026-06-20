#include "IOProcessor.h"
#include <iostream>
#include <array>

IOProcessor::IOProcessor(ProcessorManager& inProcessorManager, uint16_t inPort)
	: ProcessorBase(inProcessorManager)
    , sessionLookupTable(1000)
	, sock(INVALID_SOCKET)
    , port(inPort)
{
}

IOProcessor::~IOProcessor()
{
}

bool IOProcessor::StartImpl()
{
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		return false;
	}

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        std::cout << "bind failed with error: "<< WSAGetLastError() << std::endl;
        closesocket(sock);
        sock = INVALID_SOCKET;

        return false;
    }

	processorThread = std::jthread(&IOProcessor::RunRecvThread, this);

	return true;
}

void IOProcessor::StopImpl()
{
    if (sock != INVALID_SOCKET)
    {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}

bool IOProcessor::SendPacket(const sockaddr_in& destAddr, const char* data, int dataSize)
{
    if (sock == INVALID_SOCKET)
    {
        return false;
    }

	int result = sendto(sock, data, dataSize, 0, (const sockaddr*)&destAddr, sizeof(destAddr));
	if (result == SOCKET_ERROR)
	{
		std::cout << "sendto failed with error: " << WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

void IOProcessor::RunRecvThread()
{
    constexpr int MaxPacketSize = 4096;
    std::array<char, MaxPacketSize> recvBuffer;

    while (true)
    {
        sockaddr_in addr{};
        int addrLen = sizeof(addr);

        int ret = recvfrom(
            sock,
            recvBuffer.data(),
            static_cast<int>(recvBuffer.size()),
            0,
            reinterpret_cast<sockaddr*>(&addr),
            &addrLen);

        if (ret == SOCKET_ERROR)
        {
            const int errorCode = WSAGetLastError();

            if (needStop.load())
            {
                break;
            }

            std::cout << "recvfrom failed : " << errorCode << '\n';
            continue;
        }

        //ProcessPacket(recvBuffer.data(), ret, addr);
    }
}

void IOProcessor::ProcessTask(std::unique_ptr<ProcessorTask>&& task)
{
}
