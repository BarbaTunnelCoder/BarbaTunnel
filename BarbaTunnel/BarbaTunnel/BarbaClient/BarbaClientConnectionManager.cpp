#include "StdAfx.h"
#include "BarbaClientConnectionManager.h"
#include "BarbaClientRedirectConnection.h"
#include "BarbaClientUdpConnection.h"
#include "BarbaClientHttpConnection.h"


BarbaClientConnectionManager::BarbaClientConnectionManager(void)
{
}


BarbaClientConnectionManager::~BarbaClientConnectionManager(void)
{
}

BarbaClientConnection* BarbaClientConnectionManager::CreateConnection(PacketHelper* packet, BarbaClientConfig* config, BarbaClientConfigItem* configItem)
{
	BarbaClientConnection* conn = NULL;
	if (configItem->Mode==BarbaModeTcpRedirect || configItem->Mode==BarbaModeUdpRedirect)
	{
		conn = new BarbaClientRedirectConnection(config, configItem, packet->GetSrcPort(), configItem->GetNewTunnelPort());
	}
	else if (configItem->Mode==BarbaModeUdpTunnel)
	{
		conn = new BarbaClientUdpConnection(config, configItem, packet->GetSrcPort(), configItem->GetNewTunnelPort());
	}
	else if (configItem->Mode==BarbaModeHttpTunnel)
	{
		conn = new BarbaClientHttpConnection(config, configItem, configItem->GetNewTunnelPort());
	}
	else
	{
		throw new BarbaException(_T("%s mode not supported!"), BarbaMode_ToString(configItem->Mode));
	}

	AddConnection(conn);
	return conn;
}
