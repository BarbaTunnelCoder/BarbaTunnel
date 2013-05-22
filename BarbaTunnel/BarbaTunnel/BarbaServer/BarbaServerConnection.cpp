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

LPCTSTR BarbaServerConnection::GetName()
{
	return Config->Name.empty() ? _T("Connection") : Config->Name.data();
}

BarbaModeEnum BarbaServerConnection::GetMode()
{
	return Config->Mode;
}

void BarbaServerConnection::ReportNewConnection()
{
	TCHAR ip[50];
	PacketHelper::ConvertIpToString(ClientIp, ip, _countof(ip));
	TCHAR virtualIp[50];
	PacketHelper::ConvertIpToString(ClientVirtualIp, virtualIp, _countof(virtualIp));
	LPCTSTR mode = BarbaMode_ToString(GetMode());
	std::tstring tunnelPorts = Config->TunnelPorts.ToString();
	BarbaLog(_T("New %s! %s - %s:%s, VirtualIP: %s, ConnectionID: %u."), GetName(), ip, mode, tunnelPorts.data(), virtualIp, GetId());
	BarbaNotify(_T("New %s\r\nClient IP: %s\r\nClient Virtual IP: %s\r\nProtocol: %s:%s"), GetName(), ip, virtualIp, mode, tunnelPorts.data());
}

