#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"
#include "BarbaCrypt.h"

BarbaServerConnection::BarbaServerConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, u_long clientIp)
	: BarbaConnection()
{
	this->ConfigItem = configItem;
	this->ClientVirtualIp = clientVirtualIp;
	this->ClientIp = clientIp;
}

SimpleBuffer* BarbaServerConnection::GetKey()
{
	return &theServerApp->Config.Key;
}

u_long BarbaServerConnection::GetClientVirtualIp()
{
	return this->ClientVirtualIp;
}

LPCTSTR BarbaServerConnection::GetName()
{
	return _tcslen(this->ConfigItem->Name)>0 ? this->ConfigItem->Name : _T("Connection");
}

BarbaModeEnum BarbaServerConnection::GetMode()
{
	return this->ConfigItem->Mode;
}

void BarbaServerConnection::ReportNewConnection()
{
	TCHAR ip[50];
	PacketHelper::ConvertIpToString(this->ClientIp, ip, _countof(ip));
	TCHAR virtualIp[50];
	PacketHelper::ConvertIpToString(this->ClientVirtualIp, virtualIp, _countof(virtualIp));
	LPCTSTR mode = BarbaMode_ToString(GetMode());
	BarbaLog(_T("New %s! %s - %s:%d, Virtual IP: %s"), this->GetName(), ip, mode, this->GetTunnelPort(), virtualIp);
	BarbaNotify(_T("New %s\r\nClient IP: %s\r\nClient Virtual IP: %s\r\nProtocol: %s:%d"), this->GetName(), ip, virtualIp, mode, this->GetTunnelPort());
}

