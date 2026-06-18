#include "IOProcessor.h"
#include <iostream>
#include <array>

IOProcessor::IOProcessor()
	: sessionLookupTable(1000)
{
}

IOProcessor::~IOProcessor()
{
}

bool IOProcessor::Start()
{
	if (not ProcessorBase::Start())
    {
        return false;
    }

	recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (recvSocket == INVALID_SOCKET || sendSocket == INVALID_SOCKET)
	{
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		ProcessorBase::Stop();

		return false;
	}

	processorThread = std::jthread(&IOProcessor::RunRecvThread, this);

	return true;
}

void IOProcessor::Stop()
{
	needStop.store(true);

    if (recvSocket != INVALID_SOCKET)
    {
        closesocket(recvSocket);
        recvSocket = INVALID_SOCKET;
    }
    
    if (sendSocket != INVALID_SOCKET)
    {
        closesocket(sendSocket);
        sendSocket = INVALID_SOCKET;
	}

    ProcessorBase::Stop();
}

bool IOProcessor::SendPacket(const sockaddr_in& destAddr, const char* data, int dataSize)
{
	int result = sendto(sendSocket, data, dataSize, 0, (const sockaddr*)&destAddr, sizeof(destAddr));
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
            recvSocket,
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
