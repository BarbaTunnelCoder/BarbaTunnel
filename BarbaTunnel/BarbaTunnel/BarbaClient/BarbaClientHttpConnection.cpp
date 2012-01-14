#include "StdAfx.h"
#include "BarbaClientHttpConnection.h"
#include "BarbaClientApp.h"


BarbaClientHttpConnection::BarbaClientHttpConnection(BarbaClientConfig* config, u_short tunnelPort)
	: BarbaClientConnection(config)
	, Courier()
{
	this->SessionId = BarbaUtils::GetRandom(100000, UINT_MAX-1);
	this->TunnelPort = tunnelPort;

	BarbaCourierCreateStrcut cs = {0};
	cs.HostName = config->ServerAddress;
	cs.RequestDataKeyName = config->RequestDataKeyName;
	cs.FakeFileMaxSize = config->FakeFileMaxSize;
	cs.FakeHttpGetTemplate = theClientApp->FakeHttpGetTemplate;
	cs.FakeHttpPostTemplate = theClientApp->FakeHttpPostTemplate;
	cs.MaxConnection = config->MaxUserConnections;
	cs.SessionId = this->SessionId;
	cs.ThreadsStackSize = BARBA_SocketThreadStackSize;
	cs.ConnectionTimeout = theApp->ConnectionTimeout;
	cs.FakePacketMinSize = config->FakePacketMinSize;
	cs.KeepAliveInterval = config->KeepAliveInterval;
	this->Courier = new BarbaClientHttpCourier(&cs, config->ServerIp, tunnelPort, this);
}

BarbaClientHttpConnection::~BarbaClientHttpConnection(void)
{
	theApp->AddThread(this->Courier->Delete());
}

bool BarbaClientHttpConnection::ShouldProcessPacket(PacketHelper* packet)
{
	//just process outgoing packets
	return packet->GetDesIp()==GetServerIp() && BarbaClientApp::ShouldGrabPacket(packet, Config);
}

bool BarbaClientHttpConnection::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		this->SetWorkingState(packet->GetIpLen(), true);
		this->Courier->SendPacket(packet);
	}
	else
	{
		this->SendPacketToMstcp(packet);
	}
	return true;
}
