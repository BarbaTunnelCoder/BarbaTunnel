#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConfig.h"

BarbaServerConfigItem::BarbaServerConfigItem()
{
	this->RealPort = 0;
}

BarbaServerConfig::BarbaServerConfig()
{
	VirtualIpRange.EndIp = 0;
	VirtualIpRange.StartIp = 0;
	ItemsCount = 0;
}


bool BarbaServerConfig::LoadFile(LPCTSTR file)
{
	AutoStartDelayMinutes = GetPrivateProfileInt(_T("General"), _T("AutoStartDelayMinutes"), 0, file);

	//Key
	TCHAR hexKey[BARBA_MAX_KEYLEN*2];
	GetPrivateProfileString(_T("General"), _T("Key"), _T(""), hexKey, _countof(hexKey), file);
	BarbaUtils::ConvertHexStringToBuffer(hexKey, &this->Key);

	//VirtualIpRange
	TCHAR virtualIpRange[100] = {0};
	GetPrivateProfileString(_T("General"), _T("VirtualIpRange"), _T(""), virtualIpRange, _countof(virtualIpRange), file);
	TCHAR* dash = _tcschr(virtualIpRange, '-');
	
	TCHAR ipBuffer[100];
	_tcsncpy_s(ipBuffer, _countof(ipBuffer), virtualIpRange, dash!=NULL ? dash-virtualIpRange : _tcslen(virtualIpRange));
	this->VirtualIpRange.StartIp = PacketHelper::ConvertStringIp(ipBuffer);
	this->VirtualIpRange.EndIp = this->VirtualIpRange.StartIp + 0xFFFF; //default
	if (dash!=NULL)
	{
		_tcscpy_s(ipBuffer, _countof(ipBuffer), dash+1);
		this->VirtualIpRange.EndIp = PacketHelper::ConvertStringIp(ipBuffer);
	}

	//Items
	//load Items
	int notfoundCounter = 0;
	this->ItemsCount = 0;
	for (int i=0; notfoundCounter<4; i++)
	{
		//create section name [Item1, Item2, ....]
		TCHAR sectionName[50];
		_stprintf_s(sectionName, _countof(sectionName), _T("Item%d"), i+1);

		//read item
		BarbaServerConfigItem* item = &this->Items[this->ItemsCount];
		bool load = item->Load(sectionName, file);
		if (!load)
		{
			notfoundCounter++;
			continue;
		}

		//ListenPorts
		TCHAR tunnelPorts[BARBA_MAX_PORTITEM*10];
		GetPrivateProfileString(sectionName, _T("TunnelPorts"), _T(""), tunnelPorts, _countof(tunnelPorts), file);
		item->TunnelPortsCount = BarbaUtils::ParsePortRanges(tunnelPorts, item->TunnelPorts, _countof(item->TunnelPorts));
		this->ItemsCount++;
	}

	return true;
}

