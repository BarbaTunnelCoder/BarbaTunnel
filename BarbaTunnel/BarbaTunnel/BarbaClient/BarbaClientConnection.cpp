#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaCrypt.h"

BarbaClientConnection::BarbaClientConnection(BarbaClientConfig* config)
{
	this->Config = config;
}

BarbaModeEnum BarbaClientConnection::GetMode()
{
	return this->Config->Mode;
}

LPCTSTR BarbaClientConnection::GetName()
{
	return this->Config->Name.empty() ? _T("Connection") : this->Config->Name.data();
}

BarbaBuffer* BarbaClientConnection::GetKey()
{
	return &this->Config->Key;
}

u_long BarbaClientConnection::GetServerIp()
{
	return this->Config->ServerIp;
}

void BarbaClientConnection::ReportNewConnection()
{
	LPCTSTR mode = BarbaMode_ToString(GetMode());
	BarbaLog(_T("New %s! Server: %s, Protocol: %s:%d, ConnectionID: %u."), GetName(), this->Config->ServerAddress.data(), mode, this->GetTunnelPort(), this->GetId());
	BarbaNotify(_T("New %s\r\nServer: %s\r\nProtocol: %s:%d"), GetName(), this->Config->ServerAddress.data(), mode, this->GetTunnelPort());
}

