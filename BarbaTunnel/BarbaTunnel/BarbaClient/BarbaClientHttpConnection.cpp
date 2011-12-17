#include "StdAfx.h"
#include "BarbaClientHttpConnection.h"


BarbaClientHttpConnection::BarbaClientHttpConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem, u_short tunnelPort)
	: BarbaClientConnection(config, configItem)
	, Courier(config->ServerIp, tunnelPort, 4)
{
	this->TunnelPort = tunnelPort;
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
