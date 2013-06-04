#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaCrypt.h"

BarbaClientConnection::BarbaClientConnection(BarbaClientConfig* config)
{
	Config = config;
}

BarbaModeEnum BarbaClientConnection::GetMode()
{
	return Config->Mode;
}

BarbaBuffer* BarbaClientConnection::GetKey()
{
	return &Config->Key;
}

u_long BarbaClientConnection::GetServerIp()
{
	return Config->ServerIp;
}

void BarbaClientConnection::ReportNewConnection()
{
	std::tstring serverAddress = Config->ServerAddress;
	if (theApp->LogAnonymously)
		serverAddress = BarbaUtils::ConvertIpToString(Config->ServerIp, true);

	LPCTSTR mode = BarbaMode_ToString(GetMode());
	std::tstring tunnelPorts = Config->TunnelPorts.ToString();
	BarbaLog(_T("New %s! Server: %s, Protocol: %s:%s, ConnectionID: %u."), Config->GetName(theApp->LogAnonymously).data(), serverAddress.data(), mode, tunnelPorts.data(), GetId());
	BarbaNotify(_T("New %s\r\nServer: %s\r\nProtocol: %s:%s"), Config->GetName(false).data(), Config->ServerAddress.data(), mode, tunnelPorts.data());
}

