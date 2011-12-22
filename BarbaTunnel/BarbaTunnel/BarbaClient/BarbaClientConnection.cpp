#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaCrypt.h"

BarbaClientConnection::BarbaClientConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem)
{
	this->Config = config;
	this->ConfigItem = configItem;
}

BarbaModeEnum BarbaClientConnection::GetMode()
{
	return this->ConfigItem->Mode;
}

LPCTSTR BarbaClientConnection::GetName()
{
	return _tcslen(this->ConfigItem->Name )>0 ? this->ConfigItem->Name : _T("Connection");
}

SimpleBuffer* BarbaClientConnection::GetKey()
{
	return &this->Config->Key;
}

u_long BarbaClientConnection::GetServerIp()
{
	return this->Config->ServerIp;
}

void BarbaClientConnection::ReportNewConnection()
{
	TCHAR serverIp[100];
	PacketHelper::ConvertIpToString(Config->ServerIp, serverIp, _countof(serverIp));
	LPCTSTR mode = BarbaMode_ToString(GetMode());
	TCHAR serverName[BARBA_MAX_CONFIGNAME];
	if (_tcslen(Config->ServerName)>0)
		_stprintf_s(serverName, _T("%s (%s)"), Config->ServerName, serverIp);
	else
		_stprintf_s(serverName, _T("%s"), serverIp);
	BarbaLog(_T("New %s, Server: %s, Protocol: %s:%d, Connection ID: %u."), GetName(), serverName, mode, this->GetTunnelPort(), this->GetId());
	BarbaNotify(_T("New %s\r\nServer: %s\r\nProtocol: %s:%d"), GetName(), serverName, mode, this->GetTunnelPort());
}

