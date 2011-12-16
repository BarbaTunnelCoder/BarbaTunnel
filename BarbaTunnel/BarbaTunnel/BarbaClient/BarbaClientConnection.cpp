#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaCrypt.h"

BarbaClientConnection::BarbaClientConnection(LPCTSTR connectionName, BarbaKey* key)
	: BarbaConnection(connectionName, key)
{
	this->Config = NULL;
	this->ConfigItem = NULL;
	this->ClientPort = 0;
	this->TunnelPort = 0;
	this->OrgClientPort = 0;
}


void BarbaClientConnection::ReportNewConnection()
{
	TCHAR serverIp[100];
	PacketHelper::ConvertIpToString(Config->ServerIp, serverIp, _countof(serverIp));
	LPCTSTR mode = BarbaMode_ToString(GetMode());
	LPCTSTR connectionName = _tcslen(GetConnectionName())>0 ? GetConnectionName() : _T("Connection");
	TCHAR serverName[BARBA_MAX_CONFIGNAME];
	if (_tcslen(Config->ServerName)>0)
		_stprintf_s(serverName, _T("%s (%s)"), Config->ServerName, serverIp);
	else
		_stprintf_s(serverName, _T("%s"), serverIp);
	BarbaLog(_T("New %s, Server: %s, Protocol: %s:%d"), connectionName, serverName, mode, this->TunnelPort);
	BarbaNotify(_T("New %s\r\nServer: %s\r\nProtocol: %s:%d"), connectionName, serverName, mode, this->TunnelPort);
}

