#include "StdAfx.h"
#include "BarbaClientHttpConnection.h"
#include "BarbaClientApp.h"


BarbaClientHttpConnection::BarbaClientHttpConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem, u_short tunnelPort)
	: BarbaClientConnection(config, configItem)
	, Courier()
{
	this->TunnelPort = tunnelPort;
	this->Courier = new BarbaClientHttpCourier(configItem->MaxUserConnections, config->ServerIp, tunnelPort, 
		theClientApp->FakeHttpGetTemplate.data(), theClientApp->FakeHttpPostTemplate.data(), this);
}

BarbaClientHttpConnection::~BarbaClientHttpConnection(void)
{
	theApp->AddThread(this->Courier->Delete());
}

bool BarbaClientHttpConnection::ShouldProcessPacket(PacketHelper* packet)
{
	//just process outgoing packets
	return packet->GetDesIp()==GetServerIp() && ConfigItem->ShouldGrabPacket(packet);
}

bool BarbaClientHttpConnection::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		this->Courier->SendPacket(packet);
	}
	else
	{
		this->SendPacketToMstcp(packet);
	}
	return true;
}
