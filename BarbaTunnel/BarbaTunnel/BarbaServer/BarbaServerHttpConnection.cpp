#include "StdAfx.h"
#include "BarbaServerHttpConnection.h"
#include "BarbaServerApp.h"

BarbaServerHttpConnection::BarbaServerHttpConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, u_long clientIp, u_short tunnelPort, u_long sessionId)
	: BarbaServerConnection(configItem, clientVirtualIp, clientIp)
{
	this->ClientLocalIp = 0;
	this->SessionId = sessionId;
	this->TunnelPort = tunnelPort;

	BarbaCourierCreateStrcut cs = {0};
	cs.FakeFileHeaderSizeKeyName = configItem->FakeFileHeaderSizeKeyName.data();
	cs.FakeFileMaxSize = configItem->FakeFileMaxSize;
	cs.SessionKeyName = configItem->SessionKeyName.data();
	cs.FakeHttpGetTemplate = theServerApp->FakeHttpGetReplyTemplate.data();
	cs.FakeHttpPostTemplate = theServerApp->FakeHttpPostReplyTemplate.data();
	cs.MaxConnenction = configItem->MaxUserConnections;
	cs.SessionId = sessionId;
	cs.ThreadsStackSize = BARBA_SocketThreadStackSize;
	this->Courier = new BarbaServerHttpCourier(&cs , this);
}

BarbaServerHttpConnection::~BarbaServerHttpConnection(void)
{
	theApp->AddThread(this->Courier->Delete());
}

bool BarbaServerHttpConnection::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		packet->SetDesIp(this->ClientLocalIp);
		packet->RecalculateChecksum();
		this->SetWorkingState(packet->GetIpLen(), true);
		this->Courier->SendPacket(packet);
	}
	else
	{
		//Initialize First Attempt
		if (this->ClientLocalIp==0)
			this->ClientLocalIp = packet->GetSrcIp();

		packet->SetSrcIp(this->ClientVirtualIp);
		this->SendPacketToMstcp(packet);
	}

	return true;
}

bool BarbaServerHttpConnection::AddSocket(BarbaSocket* Socket, LPCSTR httpRequest, bool isOutgoing)
{
	return this->Courier->AddSocket(Socket, httpRequest, isOutgoing);
}

bool BarbaServerHttpConnection::ShouldProcessPacket(PacketHelper* packet)
{
	//just process outgoing packets
	return packet->GetDesIp()==this->GetClientVirtualIp();
}
