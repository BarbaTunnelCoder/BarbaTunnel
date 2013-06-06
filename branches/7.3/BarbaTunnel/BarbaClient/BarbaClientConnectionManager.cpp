#include "StdAfx.h"
#include "BarbaClientConnectionManager.h"
#include "BarbaClientRedirectConnection.h"
#include "BarbaClientUdpConnection.h"
#include "BarbaClientTcpConnection.h"
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
		conn = new BarbaClientRedirectConnection(config, packet->GetSrcPort(), config->TunnelPorts.GetRandomPort());
	}
	else if (config->Mode==BarbaModeUdpTunnel)
	{
		conn = new BarbaClientUdpConnection(config, packet->GetSrcPort(), config->TunnelPorts.GetRandomPort());
	}
	else if (config->Mode==BarbaModeTcpTunnel)
	{
		BarbaClientTcpConnection* tcpConn = new BarbaClientTcpConnection(config);
		tcpConn->Init();
		conn = tcpConn;
	}
	else if (config->Mode==BarbaModeHttpTunnel)
	{
		BarbaClientHttpConnection* httpConn = new BarbaClientHttpConnection(config);
		httpConn->Init();
		conn = httpConn;
	}
	else
	{
		throw new BarbaException(_T("%s mode not supported!"), BarbaMode_ToString(config->Mode));
	}

	AddConnection(conn);
	return conn;
}
