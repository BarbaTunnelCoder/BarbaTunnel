#include "StdAfx.h"
#include "BarbaServerConnectionManager.h"
#include "BarbaServerUdpConnection.h"
#include "BarbaServerRedirectConnection.h"
#include "BarbaServerApp.h"


BarbaServerConnectionManager::BarbaServerConnectionManager(void)
{
}


BarbaServerConnectionManager::~BarbaServerConnectionManager(void)
{
}

void BarbaServerConnectionManager::Initialize(IpRange* virtualIpRange)
{
	this->VirtualIpManager.Initialize(virtualIpRange);
}

BarbaServerConnection* BarbaServerConnectionManager::FindByVirtualIp(DWORD ip)
{
	BarbaServerConnection* ret = NULL;
	BarbaServerConnection** connections = (BarbaServerConnection**)this->Connections.LockBuffer();
	for (size_t i=0; i<this->Connections.GetCount() && ret==NULL; i++)
	{
		BarbaServerConnection* conn = connections[i];
		if (conn->ClientVirtualIp==ip )
			ret = conn;
	}

	this->Connections.UnlockBuffer();
	return ret;
}

BarbaServerConnection* BarbaServerConnectionManager::FindBySessionId(DWORD sessionId)
{
	BarbaServerConnection* ret = NULL;
	BarbaServerConnection** connections = (BarbaServerConnection**)this->Connections.LockBuffer();
	for (size_t i=0; i<this->Connections.GetCount() && ret==NULL; i++)
	{
		BarbaServerConnection* conn = connections[i];
		if (conn->GetSessionId()==sessionId)
			ret = conn;
	}
	this->Connections.UnlockBuffer();
	return ret;
}
	

BarbaServerConnection* BarbaServerConnectionManager::Find(DWORD clientIp, u_short clientPort, BarbaModeEnum mode)
{
	BarbaServerConnection* ret = NULL;
	BarbaServerConnection** connections = (BarbaServerConnection**)this->Connections.LockBuffer();
	for (size_t i=0; i<this->Connections.GetCount() && ret==NULL; i++)
	{
		BarbaServerConnection* conn = connections[i];
		if (conn->ClientIp==clientIp && conn->ClientPort==clientPort && conn->GetMode()==mode)
			ret = conn;
	}
	this->Connections.UnlockBuffer();
	return ret;
}


BarbaServerConnection* BarbaServerConnectionManager::CreateConnection(PacketHelper* packet, BarbaServerConfigItem* configItem)
{
	CleanTimeoutConnections();

	BarbaServerConnection* conn = NULL;
	if (configItem->Mode==BarbaModeUdpRedirect || configItem->Mode==BarbaModeTcpRedirect)
	{
		conn = new BarbaServerRedirectConnection(configItem->Name, &theServerApp->Config.Key, configItem->RealPort, configItem->Mode);
	}
	else if (configItem->Mode==BarbaModeUdpTunnel)
	{
		conn = new BarbaServerUdpConnection(configItem->Name, &theServerApp->Config.Key);
		memcpy_s(conn->ClientEthAddress, ETH_ALEN, packet->ethHeader->h_source, ETH_ALEN);
		conn->ClientIp = packet->GetSrcIp(); 
		conn->ClientVirtualIp = this->VirtualIpManager.GetNewIp();
		conn->ClientPort = packet->GetSrcPort();
		conn->ClientTunnelPort = packet->GetDesPort();
	}
	else
	{
		throw _T("Unsupported mode!");
	}


	conn->ReportNewConnection();
	Connections.AddTail(conn);
	return conn;
}
