#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"
#include "BarbaCrypt.h"

BarbaServerConnection::BarbaServerConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp)
	: BarbaConnection()
{
	Config = config;
	ClientVirtualIp = clientVirtualIp;
	ClientIp = clientIp;
}

BarbaServerConnection::~BarbaServerConnection()
{
}


BarbaBuffer* BarbaServerConnection::GetKey()
{
	return &Config->Key;
}

u_long BarbaServerConnection::GetClientVirtualIp()
{
	return ClientVirtualIp;
}

BarbaModeEnum BarbaServerConnection::GetMode()
{
	return Config->Mode;
}

void BarbaServerConnection::ReportNewConnection()
{
	std::tstring ip = BarbaUtils::ConvertIpToString(ClientIp, theApp->LogAnonymously);
	std::tstring virtualIp = BarbaUtils::ConvertIpToString(ClientVirtualIp, false);
	LPCTSTR mode = BarbaMode_ToString(GetMode());
	std::tstring tunnelPorts = Config->TunnelPorts.ToString();

	BarbaLog(_T("New %s! %s - %s:%s, VirtualIP: %s, ConnectionID: %u."), Config->GetName(theApp->LogAnonymously).data(), BarbaUtils::ConvertIpToString(ClientIp, theApp->LogAnonymously).data(), mode, tunnelPorts.data(), virtualIp.data(), GetId());
	BarbaNotify(_T("New %s\r\nClient IP: %s\r\nClient Virtual IP: %s\r\nProtocol: %s:%s"), Config->GetName(false).data(), BarbaUtils::ConvertIpToString(ClientIp, false).data(), virtualIp.data(), mode, tunnelPorts.data());
}

