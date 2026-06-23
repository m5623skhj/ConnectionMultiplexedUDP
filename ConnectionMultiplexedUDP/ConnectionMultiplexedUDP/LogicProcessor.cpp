#include "LogicProcessor.h"
#include "ProcessorManager.h"
#include "Generated/PacketEnvelope.pb.h"
#include "Protocol/PacketProtocol.h"
#include "ReceivedPacketTask.h"

LogicProcessor::LogicProcessor(ProcessorManager& inProcessorManager)
    : ProcessorBase(inProcessorManager)
{
}

LogicProcessor::~LogicProcessor()
{
	Stop();
}

bool LogicProcessor::StartImpl()
{
	return true;
}

void LogicProcessor::StopImpl()
{
}

void LogicProcessor::ProcessTask(std::unique_ptr<ProcessorTaskBase>&& task)
{
	auto* receivedPacketTask = dynamic_cast<ReceivedPacketTask*>(task.get());
	if (receivedPacketTask == nullptr)
	{
		return;
	}

	ProcessReceivedPacket(*receivedPacketTask);
}

void LogicProcessor::ProcessReceivedPacket(const ReceivedPacketTask& task)
{
	const std::vector<char>& packetData = task.GetPacketData();
	if (packetData.empty())
	{
		return;
	}

	cmudp::protocol::PacketEnvelope envelope;
	if (not envelope.ParseFromArray(packetData.data(), static_cast<int>(packetData.size())))
	{
		return;
	}
	if (envelope.connection_id() == InvalidConnectionId
		|| envelope.authenticated_data().empty()
		|| envelope.authentication_tag().size() != cmudp::protocol::AUTHENTICATION_TAG_SIZE)
	{
		return;
	}

	processorManager.AuthenticateAndDispatchPacket(
		task.GetSenderAddress(),
		envelope.connection_id(),
		envelope.authenticated_data(),
		envelope.authentication_tag());
}
