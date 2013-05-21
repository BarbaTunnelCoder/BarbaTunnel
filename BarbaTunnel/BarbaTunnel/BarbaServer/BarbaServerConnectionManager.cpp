#include "StdAfx.h"
#include "BarbaServerConnectionManager.h"
#include "BarbaServerUdpConnection.h"
#include "BarbaServerRedirectConnection.h"
#include "BarbaServerApp.h"
#include "BarbaServerTcpConnection.h"
#include "BarbaServerHttpConnection.h"


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

void BarbaServerConnectionManager::RemoveConnection(BarbaConnection* conn)
{
	u_long ip = ((BarbaServerConnection*)conn)->GetClientVirtualIp();
	BarbaConnectionManager::RemoveConnection(conn);
	VirtualIpManager.ReleaseIp(ip);
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

BarbaServerConnection* BarbaServerConnectionManager::FindBySessionId(u_long sessionId)
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

BarbaServerTcpConnectionBase* BarbaServerConnectionManager::CreateTcpConnection(BarbaServerConfig* config, u_long clientIp, LPCTSTR requestData)
{
	BarbaCourierRequestMode requestMode;
	requestMode.Parse(BarbaUtils::GetKeyValueFromString(requestData, _T("HttpRequestMode")));
	if (requestMode.Mode==BarbaCourierRequestMode::ModeNone && !config->AllowHttpRequestNone) throw new BarbaException("Server does not accept Simple TCP connection!");
	if (requestMode.Mode==BarbaCourierRequestMode::ModeNormal && !config->AllowHttpRequestNormal) throw new BarbaException("Server does not accept Normal HTTP connection!");
	if (requestMode.Mode==BarbaCourierRequestMode::ModeBombard && !config->AllowHttpRequestBombard) throw new BarbaException("Server does not accept Bombard HTTP connection!");

	BarbaServerTcpConnectionBase* conn =  requestMode.Mode == BarbaCourierRequestMode::ModeNone
		? (BarbaServerTcpConnectionBase*)new BarbaServerTcpConnection(config, this->VirtualIpManager.GetNewIp(), clientIp)
		: (BarbaServerTcpConnectionBase*)new BarbaServerHttpConnection(config, this->VirtualIpManager.GetNewIp(), clientIp);
	conn->Init(requestData);
	AddConnection(conn);
	return conn;

}
