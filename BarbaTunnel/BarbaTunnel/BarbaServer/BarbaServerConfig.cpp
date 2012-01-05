#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConfig.h"

BarbaServerConfigItem::BarbaServerConfigItem()
{
	this->RealPort = 0;
}

BarbaServerConfig::BarbaServerConfig()
{
}

bool BarbaServerConfig::LoadFile(LPCTSTR file)
{
	if (!BarbaUtils::IsFileExists(file))
		return false;

	//ServerIp
	TCHAR serverAddress[255];
	GetPrivateProfileString(_T("General"), _T("ServerAddress"), _T(""), serverAddress, _countof(serverAddress), file);
	this->ServerIp = PacketHelper::ConvertStringIp(serverAddress);

	//Name
	GetPrivateProfileString(_T("General"), _T("ServerName"), _T(""), ServerName, _countof(ServerName), file);

	//load Items
	int notfoundCounter = 0;
	for (int i=0; notfoundCounter<4; i++)
	{
		//create section name [Item1, Item2, ....]
		TCHAR sectionName[50];
		_stprintf_s(sectionName, _countof(sectionName), _T("Item%d"), i+1);

		//read item
		BarbaServerConfigItem item;
		bool load = item.Load(sectionName, file);
		if (!load)
		{
			notfoundCounter++;
			continue;
		}

		this->Items.push_back(item);
	}

	return true;
}

