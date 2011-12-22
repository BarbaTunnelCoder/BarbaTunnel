#include "StdAfx.h"
#include "BarbaServerHttpCourier.h"
#include "BarbaServerHttpConnection.h"


BarbaServerHttpCourier::BarbaServerHttpCourier(u_short maxConnection, BarbaServerHttpConnection* httpConnection)
	: HttpConnection(httpConnection)
	, BarbaCourierServer(maxConnection)
{
}


BarbaServerHttpCourier::~BarbaServerHttpCourier(void)
{
}

void BarbaServerHttpCourier::SendPacket(PacketHelper* packet)
{
	//encrypt whole packet include header
	BYTE data[MAX_ETHER_FRAME];
	size_t dataCount = packet->GetPacketLen();
	memcpy_s(data, sizeof data, packet->ipHeader, dataCount);
	this->HttpConnection->CryptData(data, dataCount);
	this->Send(data, dataCount);
}

void BarbaServerHttpCourier::Receive(BYTE* buffer, size_t bufferCount)
{
	this->HttpConnection->CryptData(buffer, bufferCount);
	PacketHelper packet;
	packet.SetIpPacket((iphdr_ptr)buffer);
	this->HttpConnection->ProcessPacket(&packet, false);
}

