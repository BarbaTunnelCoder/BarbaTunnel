#include "StdAfx.h"
#include "BarbaClientHttpCourier.h"
#include "BarbaClientHttpConnection.h"


BarbaClientHttpCourier::BarbaClientHttpCourier(u_short maxConnenction, DWORD remoteIp, u_short remotePort, 
	LPCSTR fakeHttpGetTemplate, LPCSTR fakeHttpPostTemplate, 
	BarbaClientHttpConnection* httpConnection)
	: HttpConnection(httpConnection)
	, BarbaCourierClient(maxConnenction, remoteIp, remotePort, fakeHttpGetTemplate, fakeHttpPostTemplate)
{
}


BarbaClientHttpCourier::~BarbaClientHttpCourier(void)
{
}

void BarbaClientHttpCourier::Receive(BYTE* buffer, size_t bufferCount)
{
	//printf("BarbaClientHttpCourier::Receive %d\n", bufferCount);
	this->HttpConnection->CryptData(buffer, bufferCount);
	PacketHelper packet;
	packet.SetIpPacket((iphdr_ptr)buffer);
	this->HttpConnection->ProcessPacket(&packet, false);
}

void BarbaClientHttpCourier::SendPacket(PacketHelper* packet)
{
	//encrypt whole packet include header
	BYTE data[MAX_ETHER_FRAME];
	size_t dataCount = packet->GetPacketLen();
	memcpy_s(data, sizeof data, packet->ipHeader, dataCount);
	this->HttpConnection->CryptData(data, dataCount);
	//printf("BarbaClientHttpCourier::Send %d\n", dataCount);
	this->Send(data, dataCount);
}
