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
	

BarbaServerConnection* BarbaServerConnectionManager::CreateConnection(PacketHelper* packet, BarbaServerConfig* config)
{
	BarbaServerConnection* conn = NULL;
	if (config->Mode==BarbaModeUdpRedirect || config->Mode==BarbaModeTcpRedirect)
	{
		conn = new BarbaServerRedirectConnection(config, this->VirtualIpManager.GetNewIp(), packet->GetSrcIp(), packet->GetSrcPort(), packet->GetDesPort());
	}
	else if (config->Mode==BarbaModeUdpTunnel)
	{
		conn = new BarbaServerUdpConnection(config, this->VirtualIpManager.GetNewIp(), packet->GetSrcIp(), packet->GetSrcPort(), packet->GetDesPort(), packet->ethHeader->h_source);
	}
	else
	{
		throw new BarbaException(_T("%s mode not supported!"), BarbaMode_ToString(config->Mode));
	}


	AddConnection(conn);
	return conn;
}

BarbaServerHttpConnection* BarbaServerConnectionManager::CreateHttpConnection(BarbaServerConfig* config, u_long clientIp, u_short tunnelPort, u_long sessionId)
{
	BarbaServerHttpConnection* conn = new BarbaServerHttpConnection(config, this->VirtualIpManager.GetNewIp(), clientIp, tunnelPort, sessionId);
	AddConnection(conn);
	return conn;

}
