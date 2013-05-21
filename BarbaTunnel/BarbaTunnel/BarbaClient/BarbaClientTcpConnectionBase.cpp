#include "StdAfx.h"
#include "BarbaClientTcpConnectionBase.h"
#include "BarbaClientApp.h"


BarbaClientTcpConnectionBase::BarbaClientTcpConnectionBase(BarbaClientConfig* config) 
	: BarbaClientConnection(config)
{
	SessionId = BarbaUtils::GetRandom(100000, UINT_MAX-1);
	_Courier = NULL;
}

void BarbaClientTcpConnectionBase::InitHelper(BarbaCourierTcpClient::CreateStrcutTcp* cs)
{
	cs->HostName = Config->ServerAddress;
	cs->RequestDataKeyName = Config->RequestDataKeyName;
	cs->MaxTransferSize = Config->MaxTransferSize;
	cs->MaxConnections = Config->MaxUserConnections;
	cs->SessionId = SessionId;
	cs->ConnectionTimeout = theApp->ConnectionTimeout;
	cs->MinPacketSize = Config->MinPacketSize;
	cs->KeepAliveInterval = Config->KeepAliveInterval;
	cs->RemoteIp = Config->ServerIp;
	cs->PortRange = &Config->TunnelPorts;
}

BarbaClientTcpConnectionBase::~BarbaClientTcpConnectionBase(void)
{
	theApp->AddThread(GetCourier()->Delete());
}

bool BarbaClientTcpConnectionBase::ShouldProcessPacket(PacketHelper* packet)
{
	//just process outgoing packets
	return packet->GetDesIp()==GetServerIp() && BarbaClientApp::ShouldGrabPacket(packet, Config);
}

bool BarbaClientTcpConnectionBase::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		SetWorkingState(packet->GetIpLen(), true);
	
		//encrypt whole packet include header
		BarbaBuffer data(packet->ipHeader, packet->GetIpLen());
		if (!GetCourier()->IsDisposing())
			GetCourier()->Send(&data);
	}
	else
	{
		SendPacketToInbound(packet);
	}
	return true;
}
