#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"
#include "BarbaCrypt.h"

BarbaServerConnection::BarbaServerConnection(LPCTSTR connectionName, BarbaKey* barbaKey)
	: BarbaConnection(connectionName, barbaKey)
{
	this->ClientLocalIp = 0;
	this->ClientVirtualIp = 0;
	this->ClientIp = 0;
	this->ClientPort = 0;
	this->ClientTunnelPort = 0;
	memset(this->ClientEthAddress, 0, _countof(ClientEthAddress));
}

void BarbaServerConnection::ReportNewConnection()
{
	TCHAR ip[50];
	PacketHelper::ConvertIpToString(this->ClientIp, ip, _countof(ip));
	TCHAR virtualIp[50];
	PacketHelper::ConvertIpToString(this->ClientVirtualIp, virtualIp, _countof(virtualIp));
	LPCTSTR connectionName = _tcslen(GetConnectionName())>0 ? GetConnectionName() : _T("Connection");
	LPCTSTR mode = BarbaMode_ToString(GetMode());
	BarbaLog(_T("New %s! %s - %s:%d, Virtual IP: %s"), connectionName, ip, mode, this->ClientTunnelPort, virtualIp);
	BarbaNotify(_T("New %s\r\nClient IP: %s\r\nClient Virtual IP: %s\r\nProtocol: %s:%d"), connectionName, ip, virtualIp, mode, this->ClientTunnelPort);
}
