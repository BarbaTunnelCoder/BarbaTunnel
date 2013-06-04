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

LPCTSTR BarbaClientConnection::GetName()
{
	return Config->Name.empty() ? _T("Connection") : Config->Name.data();
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
	if (!theApp->LogIpAddress)
		serverAddress = BarbaUtils::ConvertIpToString(Config->ServerIp, !theApp->LogIpAddress);

	LPCTSTR mode = BarbaMode_ToString(GetMode());
	std::tstring tunnelPorts = Config->TunnelPorts.ToString();
	BarbaLog(_T("New %s! Server: %s, Protocol: %s:%s, ConnectionID: %u."), GetName(), serverAddress.data(), mode, tunnelPorts.data(), GetId());
	BarbaNotify(_T("New %s\r\nServer: %s\r\nProtocol: %s:%s"), GetName(), Config->ServerAddress.data(), mode, tunnelPorts.data());
}

