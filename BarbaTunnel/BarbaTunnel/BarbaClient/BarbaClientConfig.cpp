#include "StdAfx.h"
#include "BarbaClientConfig.h"
#include "PacketHelper.h"

BarbaClientConfigItem::BarbaClientConfigItem()
{
	GrabProtocolsCount = 0;
	Mode = BarbaModeNone;
	TunnelPort = 0;
	RealPort = 0;
	Enabled = false;
}

u_char BarbaClientConfigItem::GetTunnelProtocol()
{
	return BarbaMode_GetProtocol(this->Mode);
}

void BarbaClientConfig::ParseGrabProtocols(BarbaClientConfigItem* item, LPCTSTR value)
{
	TCHAR buffer[1000];
	_tcscpy_s(buffer, value);

	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(buffer, _T(","), &currentPos);
		
	while (token!=NULL && item->GrabProtocolsCount<_countof(item->GrabProtocols))
	{
		ProtocolPort protocol;
		if (BarbaUtils::GetProtocolAndPort(token, &protocol.Protocol, &protocol.Port))
		{
			item->GrabProtocols[item->GrabProtocolsCount] = protocol;
			item->GrabProtocolsCount++;
		}
		token = _tcstok_s(NULL, _T(","), &currentPos);
	}
}


BarbaClientConfigManager::BarbaClientConfigManager()
{
	ConfigsCount = 0;
}


void BarbaClientConfigManager::LoadFolder(LPCTSTR folder)
{
	TCHAR file[MAX_PATH];
	WIN32_FIND_DATA findData = {0};
		
	//findData.
	_tcscpy_s(file, _countof(file), folder);
	_tcsncat_s(file, _T("\\config\\*.ini"), MAX_PATH);
	HANDLE findHandle = FindFirstFile(file, &findData);
	BOOL bfind = findHandle!=NULL;
	while (bfind)
	{
		if (ConfigsCount>=_countof(Configs))
			break;
		BarbaClientConfig* config = &Configs[ConfigsCount];
			
		TCHAR fullPath[MAX_PATH] = {0};
		_tcsncat_s(fullPath, folder, _countof(fullPath));
		_tcsncat_s(fullPath, _T("\\config\\"), _countof(fullPath));
		_tcsncat_s(fullPath, findData.cFileName, _countof(fullPath));

		if (config->LoadFile(fullPath))
			ConfigsCount++;
		bfind = FindNextFile(findHandle, &findData);
	}
	FindClose(findHandle);
}

BarbaClientConfig* BarbaClientConfigManager::FindByServerIP(DWORD ip)
{
	for (int i=0; i<ConfigsCount; i++)
		if (Configs[i].ServerIp==ip)
			return &Configs[i];
	return NULL;
}

BarbaClientConfig::BarbaClientConfig()
{
	ServerIp = 0;
	KeyCount = 0;
	ItemsCount = 0;
}

bool BarbaClientConfig::LoadFile(LPCTSTR file)
{
	TCHAR serverAddress[255];
	GetPrivateProfileString(_T("General"), _T("ServerAddress"), _T(""), serverAddress, _countof(serverAddress), file);
	this->ServerIp = PacketHelper::ConvertStringIp(serverAddress);
	if (this->ServerIp==0)
		return false;

	TCHAR hexKey[BARBA_MAX_KEYLEN*2];
	GetPrivateProfileString(_T("General"), _T("Key"), _T(""), hexKey, _countof(hexKey), file);
	this->KeyCount = BarbaUtils::ConvertHexStringToBuffer(hexKey, this->Key, _countof(this->Key));

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
		BarbaClientConfigItem* item = &this->Items[this->ItemsCount];
		item->Enabled = GetPrivateProfileInt(sectionName, "Enabled", 1, file)!=0;
		item->Mode = BarbaMode_FromString(modeString);
		item->TunnelPort = (u_short)GetPrivateProfileInt(sectionName, "TunnelPort", 0, file);
		item->RealPort = (u_short)GetPrivateProfileInt(sectionName, "RealPort", 0, file);
		if (!item->Enabled || item->Mode==BarbaModeNone)
			continue;

		//GrabProtocols
		TCHAR grabProtocols[1000];
		GetPrivateProfileString(sectionName, _T("GrabProtocols"), _T(""), grabProtocols, _countof(grabProtocols), file);
		ParseGrabProtocols(item, grabProtocols);
		this->ItemsCount++;
	}

	return this->ItemsCount>0;
}
