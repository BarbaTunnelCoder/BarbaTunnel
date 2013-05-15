#include "StdAfx.h"
#include "BarbaServerHttpConnection.h"
#include "BarbaServerApp.h"

BarbaServerHttpConnection::BarbaServerHttpConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp, u_short tunnelPort, u_long sessionId)
	: BarbaServerConnection(config, clientVirtualIp, clientIp)
{
	this->ClientLocalIp = 0;
	this->SessionId = sessionId;
	this->TunnelPort = tunnelPort;

	BarbaCourier::CreateStrcutBag cs = {0};
	cs.HostName = config->ServerAddress;
	cs.FakeFileMaxSize = config->FakeFileMaxSize;
	cs.RequestDataKeyName = config->RequestDataKeyName.data();
	cs.MaxConnection = config->MaxUserConnections;
	cs.AllowBombard = config->AllowRequestBombard;
	cs.ConnectionTimeout = theApp->ConnectionTimeout;
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
		SendPacketToInbound(packet);
	}

	return true;
}

void BarbaServerHttpConnection::Init(LPCTSTR requestData)
{
	return this->Courier->Init(requestData);
}

bool BarbaServerHttpConnection::AddSocket(BarbaSocket* Socket, LPCSTR httpRequest, LPCTSTR requestData, bool isOutgoing)
{
	return this->Courier->AddSocket(Socket, httpRequest, requestData, isOutgoing);
}

bool BarbaServerHttpConnection::ShouldProcessPacket(PacketHelper* packet)
{
	//just process outgoing packets
	return packet->GetDesIp()==this->GetClientVirtualIp();
}
