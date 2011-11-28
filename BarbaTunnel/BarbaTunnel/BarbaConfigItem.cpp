#include "stdafx.h"
#include "BarbaConfigItem.h"
#include "BarbaUtils.h"

BarbaConfigItem::BarbaConfigItem()
{
	Mode = BarbaModeNone;
	TunnelPortsCount = 0;
	memset(this->TunnelPorts, 0, sizeof(this->TunnelPorts));
	Name[0] = 0;
	Enabled = true;
	RealPort = 0;
	_TotalTunnelPortsCount = 0;
}


size_t BarbaConfigItem::GetTotalTunnelPortsCount()
{
	if (_TotalTunnelPortsCount==0)
	{
		for (size_t i=0; i<TunnelPortsCount; i++)
		{
			int count = TunnelPorts[i].EndPort - TunnelPorts[i].StartPort + 1;
			if (count>0)
				_TotalTunnelPortsCount += count;
		}
	}
	return _TotalTunnelPortsCount;
}

bool BarbaConfigItem::Load(LPCTSTR sectionName, LPCTSTR file)
{
	//fail if mode not found
	TCHAR modeString[100];
	int res = GetPrivateProfileString(sectionName, _T("Mode"), _T(""), modeString, _countof(modeString), file);
	if (res==0)
		return false;

	this->Enabled = GetPrivateProfileInt(sectionName, "Enabled", 1, file)!=0;
	this->Mode = BarbaMode_FromString(modeString);
	this->RealPort = (u_short)GetPrivateProfileInt(sectionName, "RealPort", 0, file);
	GetPrivateProfileString(sectionName, _T("Name"), _T(""), this->Name, _countof(this->Name), file);
	//TunnelPorts
	TCHAR tunnelPorts[BARBA_MAX_PORTITEM*10];
	GetPrivateProfileString(sectionName, _T("TunnelPorts"), _T(""), tunnelPorts, _countof(tunnelPorts), file);
	this->TunnelPortsCount = BarbaUtils::ParsePortRanges(tunnelPorts, this->TunnelPorts, _countof(this->TunnelPorts));
	return true;
}