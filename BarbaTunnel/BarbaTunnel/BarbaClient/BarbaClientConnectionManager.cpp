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

BarbaClientConnection* BarbaClientConnectionManager::CreateConnection(PacketHelper* packet, BarbaClientConfig* config)
{
	BarbaClientConnection* conn = NULL;
	if (config->Mode==BarbaModeTcpRedirect || config->Mode==BarbaModeUdpRedirect)
	{
		conn = new BarbaClientRedirectConnection(config, packet->GetSrcPort(), config->GetNewTunnelPort());
	}
	else if (config->Mode==BarbaModeUdpTunnel)
	{
		conn = new BarbaClientUdpConnection(config, packet->GetSrcPort(), config->GetNewTunnelPort());
	}
	else if (config->Mode==BarbaModeHttpTunnel)
	{
		conn = new BarbaClientHttpConnection(config, config->GetNewTunnelPort());
	}
	else
	{
		throw new BarbaException(_T("%s mode not supported!"), BarbaMode_ToString(config->Mode));
	}

	AddConnection(conn);
	return conn;
}
