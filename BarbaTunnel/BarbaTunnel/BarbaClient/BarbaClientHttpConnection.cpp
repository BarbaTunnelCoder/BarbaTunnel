#include "StdAfx.h"
#include "BarbaClientHttpConnection.h"
#include "BarbaClientApp.h"


BarbaClientHttpConnection::BarbaClientHttpConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem, u_short tunnelPort)
	: BarbaClientConnection(config, configItem)
	, Courier()
{
	this->TunnelPort = tunnelPort;
	this->Courier = new BarbaClientHttpCourier(config->ServerIp, tunnelPort, 4);
	this->Courier->InitFakeRequests(theClientApp->FakeHttpGetTemplate.data(), theClientApp->FakeHttpPostTemplate.data());
}

BarbaClientHttpConnection::~BarbaClientHttpConnection(void)
{
}

bool BarbaClientHttpConnection::ShouldProcessPacket(PacketHelper* packet)
{
	//just process outgoing packets
	return packet->GetDesIp()==GetServerIp() && ConfigItem->ShouldGrabPacket(packet);
}

bool BarbaClientHttpConnection::ProcessPacket(PacketHelper* /*packet*/, bool /*send*/)
{
	return true;
}
