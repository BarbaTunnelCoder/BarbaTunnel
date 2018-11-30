#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaCrypt.h"

BarbaClientConnection::BarbaClientConnection(BarbaClientConfig* config)
	: BarbaConnection(config)
{
}

void BarbaClientConnection::ReportNewConnection()
{
	std::tstring serverAddress = GetConfig()->ServerAddress;
	if (theApp->LogAnonymously)
		serverAddress = BarbaUtils::ConvertIpToString(GetConfig()->ServerIp, true);

	LPCTSTR mode = BarbaMode_ToString(GetConfig()->Mode);
	std::tstring tunnelPorts = GetConfig()->TunnelPorts.ToString();
	BarbaLog(_T("New %s! Server: %s, Protocol: %s:%s, ConnectionID: %u."), GetConfig()->GetName(theApp->LogAnonymously).data(), serverAddress.data(), mode, tunnelPorts.data(), GetId());
	BarbaNotify(_T("New %s\r\nServer: %s\r\nProtocol: %s:%s"), GetConfig()->GetName(false).data(), GetConfig()->ServerAddress.data(), mode, tunnelPorts.data());
}

