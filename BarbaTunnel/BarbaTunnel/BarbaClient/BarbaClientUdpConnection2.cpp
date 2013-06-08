#include "stdafx.h"
#include "BarbaClientUdpConnection.h"
#include "BarbaClientApp.h"


BarbaClientUdpConnection::BarbaClientUdpConnection(BarbaClientConfig* config)
	: BarbaClientConnection(config)
{
}


BarbaClientUdpConnection::~BarbaClientUdpConnection(void)
{
}

bool BarbaClientUdpConnection::ProcessOutboundPacket(PacketHelper* packet)
{
	BarbaBuffer buffer((BYTE*)packet->ipHeader, packet->GetIpLen());
	GetCourier()->SendData(&buffer);
	return true;
}

bool BarbaClientUdpConnection::ProcessInboundPacket(PacketHelper* packet)
{
	//Decrypt Packet
	DecryptPacket(packet);

	//process by courier
	//return GetCourier()->ProcessInboundPacket(packet);
	return false;
}

// GUID (32) | Control (1) | Session (4) | MessageID (4) | TotalPart (4) | PartIndex (4) | data