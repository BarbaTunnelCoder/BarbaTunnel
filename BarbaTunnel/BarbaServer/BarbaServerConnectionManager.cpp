#include "StdAfx.h"
#include "BarbaServerConnectionManager.h"
#include "BarbaServerUdpConnection.h"
#include "BarbaServerUdpSimpleConnection.h"
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
	VirtualIpManager.Initialize(virtualIpRange);
}

void BarbaServerConnectionManager::RemoveConnection(BarbaConnection* conn)
{
	u_long ip = ((BarbaServerConnection*)conn)->GetClientVirtualIp();
	BarbaConnectionManager::RemoveConnection(conn);
	VirtualIpManager.ReleaseIp(ip);
}


BarbaServerConnection* BarbaServerConnectionManager::FindByVirtualIp(DWORD ip)
{
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&Connections);
	BarbaServerConnection** connections = (BarbaServerConnection**)autoLockBuf.GetBuffer();
	for (size_t i=0; i<Connections.GetCount(); i++)
	{
		BarbaServerConnection* conn = connections[i];
		if (conn->GetClientVirtualIp()==ip )
			return conn;
	}

	return NULL;
}

BarbaServerConnection* BarbaServerConnectionManager::FindBySessionId(u_long sessionId)
{
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&Connections);
	BarbaServerConnection** connections = (BarbaServerConnection**)autoLockBuf.GetBuffer();
	for (size_t i=0; i<Connections.GetCount(); i++)
	{
		BarbaServerConnection* conn = connections[i];
		if (conn->GetSessionId()==sessionId)
			return conn;
	}
	return NULL;
}

BarbaServerConnection* BarbaServerConnectionManager::CreateConnection(BarbaServerConfig* config, PacketHelper* packet)
{
	BarbaServerConnection* conn = NULL;
	if (config->Mode==BarbaModeUdpRedirect || config->Mode==BarbaModeTcpRedirect)
	{
		conn = new BarbaServerRedirectConnection(config, VirtualIpManager.GetNewIp(), packet);
	}
	else if (config->Mode==BarbaModeUdpSimpleTunnel)
	{
		conn = new BarbaServerUdpSimpleConnection(config, VirtualIpManager.GetNewIp(), packet);
	}
	else if (config->Mode==BarbaModeUdpTunnel)
	{
		conn = new BarbaServerUdpConnection(config, VirtualIpManager.GetNewIp(), packet);
	}
	else
	{
		throw new BarbaException(_T("%s mode not supported!"), BarbaMode_ToString(config->Mode));
	}

	conn->Init();
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