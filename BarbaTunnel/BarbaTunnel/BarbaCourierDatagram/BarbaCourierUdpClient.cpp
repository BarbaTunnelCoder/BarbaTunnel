#include "stdafx.h"
#include "BarbaCourierUdpClient.h"


BarbaCourierUdpClient::BarbaCourierUdpClient(CreateStrcutUdp* cs)
	: BarbaCourierDatagram(cs)
{
}


BarbaCourierUdpClient::~BarbaCourierUdpClient(void)
{
}


// GUID (32) | Session (4) | Control (1) | MessageID (4) | TotalPart (4) | PartIndex (4) | data

bool BarbaCourierUdpClient::ProcessInboundPacket(PacketHelper* /*packet*/)
{
	std::map<int, int*> a;
	return false;
}
