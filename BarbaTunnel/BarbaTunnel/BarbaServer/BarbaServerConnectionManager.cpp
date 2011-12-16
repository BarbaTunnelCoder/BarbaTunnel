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
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&this->Connections);
	BarbaServerConnection** connections = (BarbaServerConnection**)autoLockBuf.GetBuffer();
	for (size_t i=0; i<this->Connections.GetCount() && ret==NULL; i++)
	{
		BarbaServerConnection* conn = connections[i];
		if (conn->GetClientVirtualIp()==ip )
			ret = conn;
	}

	return ret;
}

BarbaServerConnection* BarbaServerConnectionManager::FindBySessionId(DWORD sessionId)
{
	BarbaServerConnection* ret = NULL;
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&this->Connections);
	BarbaServerConnection** connections = (BarbaServerConnection**)autoLockBuf.GetBuffer();
	for (size_t i=0; i<this->Connections.GetCount() && ret==NULL; i++)
	{
		BarbaServerConnection* conn = connections[i];
		if (conn->GetSessionId()==sessionId)
			ret = conn;
	}
	return ret;
}
	

BarbaServerConnection* BarbaServerConnectionManager::CreateConnection(PacketHelper* packet, BarbaServerConfigItem* configItem)
{
	CleanTimeoutConnections();

	BarbaServerConnection* conn = NULL;
	if (configItem->Mode==BarbaModeUdpRedirect || configItem->Mode==BarbaModeTcpRedirect)
	{
		conn = new BarbaServerRedirectConnection(configItem, this->VirtualIpManager.GetNewIp(), packet->GetSrcIp(), packet->GetSrcPort(), packet->GetDesPort());
	}
	else if (configItem->Mode==BarbaModeUdpTunnel)
	{
		conn = new BarbaServerUdpConnection(configItem, this->VirtualIpManager.GetNewIp(), packet->GetSrcIp(), packet->GetSrcPort(), packet->GetDesPort(), packet->ethHeader->h_source);
	}
	else
	{
		throw _T("Unsupported mode!");
	}


	conn->ReportNewConnection();
	Connections.AddTail(conn);
	return conn;
}
