#include "StdAfx.h"
#include "BarbaClientConnectionManager.h"
#include "BarbaClientRedirectConnection.h"
#include "BarbaClientUdpConnection.h"


BarbaClientConnectionManager::BarbaClientConnectionManager(void)
{
}


BarbaClientConnectionManager::~BarbaClientConnectionManager(void)
{
}

BarbaClientConnection* BarbaClientConnectionManager::Find(DWORD serverIp, u_char tunnelProtocol, u_short clientPort)
{
	BarbaClientConnection* ret = NULL;
	BarbaClientConnection** connections = (BarbaClientConnection**)this->Connections.LockBuffer();
	for (size_t i=0; i<this->Connections.GetCount() && ret==NULL; i++)
	{
		BarbaClientConnection* conn = connections[i];
		if (conn->Config->ServerIp==serverIp && (tunnelProtocol==0 || tunnelProtocol==BarbaMode_GetProtocol(conn->ConfigItem->Mode))
			&& (clientPort==0 || clientPort==conn->ClientPort))
			ret = conn;
	}
	this->Connections.UnlockBuffer();
	return ret;
}

BarbaClientConnection* BarbaClientConnectionManager::FindByConfigItem(BarbaClientConfigItem* configItem, u_short clientPort)
{
	BarbaClientConnection* ret = NULL;
	BarbaClientConnection** connections = (BarbaClientConnection**)this->Connections.LockBuffer();
	for (size_t i=0; i<this->Connections.GetCount() && ret==NULL; i++)
	{
		BarbaClientConnection* conn = connections[i];
		if (conn->ConfigItem==configItem 
			&& (clientPort==0 || clientPort==conn->ClientPort))
			return conn;
	}
	this->Connections.UnlockBuffer();
	return ret;
}

BarbaClientConnection* BarbaClientConnectionManager::CreateConnection(PacketHelper* packet, BarbaClientConfig* config, BarbaClientConfigItem* configItem)
{
	BarbaClientConnection* conn = NULL;
	if (configItem->Mode==BarbaModeTcpRedirect || configItem->Mode==BarbaModeUdpRedirect)
	{
		conn = new BarbaClientRedirectConnection(configItem->Name, &config->Key, configItem->Mode);
	}
	else if (configItem->Mode==BarbaModeUdpTunnel)
	{
		conn = new BarbaClientUdpConnection(configItem->Name, &config->Key);
	}
	else
	{
		throw _T("Unsupported mode!");
	}

	conn->Config = config;
	conn->ConfigItem = configItem;
	conn->ClientPort = 1419;
	conn->OrgClientPort = packet->GetSrcPort();
	conn->TunnelPort = conn->ConfigItem->GetNewTunnelPort();
	AddConnection(conn);
	return conn;
}
