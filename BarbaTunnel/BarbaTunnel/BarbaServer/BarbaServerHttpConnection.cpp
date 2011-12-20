#include "StdAfx.h"
#include "BarbaServerHttpConnection.h"
#include "BarbaServerApp.h"

BarbaServerHttpConnection::BarbaServerHttpConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, u_long clientIp, u_short tunnelPort, u_long sessionId)
	: BarbaServerConnection(configItem, clientVirtualIp, clientIp)
{
	this->SessionId = sessionId;
	this->TunnelPort = tunnelPort;
	this->Courier = new BarbaServerHttpCourier(configItem->MaxUserConnections, this);
	this->Courier->InitFakeRequests(theServerApp->FakeHttpGetReplyTemplate.data(), theServerApp->FakeHttpPostReplyTemplate.data());
}

BarbaServerHttpConnection::~BarbaServerHttpConnection(void)
{
	theApp->AddThread(this->Courier->Delete());
}

bool BarbaServerHttpConnection::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		packet->SetDesIp(this->ClientIp);
		packet->RecalculateChecksum();
		this->SetWorkingState(packet->GetIpLen(), true);
		this->Courier->SendPacket(packet);
	}
	else
	{
		//printf("Received %d bytes. checksum: %d\n", packet->GetIpLen(), packet->ipHeader->ip_p);

		//Initialize First Attempt
		if (this->ClientLocalIp==0)
			this->ClientLocalIp = packet->GetSrcIp();

		//prepare for NAT
		packet->SetSrcIp(this->ClientVirtualIp);
		this->SendPacketToMstcp(packet);
	}

	return true;
}

bool BarbaServerHttpConnection::AddSocket(BarbaSocket* Socket, bool isOutgoing)
{
	return this->Courier->AddSocket(Socket, isOutgoing);
}

bool BarbaServerHttpConnection::ShouldProcessPacket(PacketHelper* packet)
{
	//just process outgoing packets
	return packet->GetDesIp()==this->GetClientVirtualIp();
}
