#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"
#include "BarbaCrypt.h"

BarbaServerConnection::BarbaServerConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp)
	: BarbaConnection()
{
	this->Config = config;
	this->ClientVirtualIp = clientVirtualIp;
	this->ClientIp = clientIp;
}

BarbaServerConnection::~BarbaServerConnection()
{
}


BarbaBuffer* BarbaServerConnection::GetKey()
{
	return &this->Config->Key;
}

u_long BarbaServerConnection::GetClientVirtualIp()
{
	return this->ClientVirtualIp;
}

LPCTSTR BarbaServerConnection::GetName()
{
	return this->Config->Name.empty() ? _T("Connection") : this->Config->Name.data();
}

BarbaModeEnum BarbaServerConnection::GetMode()
{
	return this->Config->Mode;
}

void BarbaServerConnection::ReportNewConnection()
{
	TCHAR ip[50];
	PacketHelper::ConvertIpToString(this->ClientIp, ip, _countof(ip));
	TCHAR virtualIp[50];
	PacketHelper::ConvertIpToString(this->ClientVirtualIp, virtualIp, _countof(virtualIp));
	LPCTSTR mode = BarbaMode_ToString(GetMode());
	BarbaLog(_T("New %s! %s - %s:%d, VirtualIP: %s, ConnectionID: %u."), this->GetName(), ip, mode, this->GetTunnelPort(), virtualIp, this->GetId());
	BarbaNotify(_T("New %s\r\nClient IP: %s\r\nClient Virtual IP: %s\r\nProtocol: %s:%d"), this->GetName(), ip, virtualIp, mode, this->GetTunnelPort());
}

