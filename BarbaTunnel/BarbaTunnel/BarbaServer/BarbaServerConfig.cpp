#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConfig.h"

BarbaServerConfigItem::BarbaServerConfigItem()
{
	this->Mode = BarbaModeNone;
	this->ListenPortsCount = 0;
	this->Enabled = true;
	this->RealPort = 0;
}

BarbaServerConfig::BarbaServerConfig()
{
	DebugMode = 0;
	VirtualIpRange.EndIp = 0;
	VirtualIpRange.StartIp = 0;
	KeyCount = 0;
	ItemsCount = 0;
}


bool BarbaServerConfig::LoadFile(LPCTSTR file)
{
	DebugMode = GetPrivateProfileInt(_T("General"), _T("DebugMode"), 0, file)!=0;

	//Key
	TCHAR hexKey[BARBA_MAX_KEYLEN*2];
	GetPrivateProfileString(_T("General"), _T("Key"), _T(""), hexKey, _countof(hexKey), file);
	this->KeyCount = BarbaUtils::ConvertHexStringToBuffer(hexKey, this->Key, _countof(this->Key));

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

		//fail if mode not found
		TCHAR modeString[100];
		int res = GetPrivateProfileString(sectionName, _T("Mode"), _T(""), modeString, _countof(modeString), file);
		if (res==0) 
		{
			notfoundCounter++;
			continue;
		}
		notfoundCounter = 0;

		//read item
		BarbaServerConfigItem* item = &this->Items[this->ItemsCount];
		item->Mode = BarbaMode_FromString(modeString);
		item->Enabled = GetPrivateProfileInt(sectionName, "Enabled", 1, file)!=0;
		if (!item->Enabled)
			continue;
		item->RealPort = (u_short)GetPrivateProfileInt(sectionName, "RealPort", 0, file);

		//ListenPorts
		TCHAR listenPorts[BARBA_MAX_PORTITEM*10];
		GetPrivateProfileString(sectionName, _T("ListenPorts"), _T(""), listenPorts, _countof(listenPorts), file);
		ParseListenPorts(item, listenPorts);
		this->ItemsCount++;
	}

	return true;
}

void BarbaServerConfig::ParseListenPorts(BarbaServerConfigItem* item, LPCTSTR value)
{
	item->ListenPortsCount = 0;
	
	TCHAR buffer[1000];
	_tcscpy_s(buffer, value);

	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(buffer, _T(","), &currentPos);
		
	while (token!=NULL && item->ListenPortsCount<_countof(item->ListenPorts))
	{
		PortRange* portRange = &item->ListenPorts[item->ListenPortsCount];
		if (BarbaUtils::GetPortRange(token, &portRange->StartPort, &portRange->EndPort))
		{
			item->ListenPortsCount++;
		}
		token = _tcstok_s(NULL, _T(","), &currentPos);
	}
}
