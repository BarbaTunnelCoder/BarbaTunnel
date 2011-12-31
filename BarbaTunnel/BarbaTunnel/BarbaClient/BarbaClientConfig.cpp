#include "StdAfx.h"
#include "BarbaClientConfig.h"
#include "BarbaUtils.h"

BarbaClientConfigItem::BarbaClientConfigItem()
{
}

bool BarbaClientConfigItem::ShouldGrabPacket(PacketHelper* packet)
{
	//check RealPort for redirect modes
	if (this->Mode==BarbaModeTcpRedirect || this->Mode==BarbaModeUdpRedirect)
	{
		return packet->GetDesPort()==this->RealPort;
	}

	for (size_t i=0; i<this->GrabProtocols.size(); i++)
	{
		//check GrabProtocols for tunnel modes
		ProtocolPort* protocolPort = &this->GrabProtocols[i];
		if (protocolPort->Protocol==0 || protocolPort->Protocol==packet->ipHeader->ip_p)
		{
			if (protocolPort->Port==0 || protocolPort->Port==packet->GetDesPort())
				return true;
		}
	}

	return false;
}

u_short BarbaClientConfigItem::GetNewTunnelPort()
{
	int newPortIndex = (rand()*rand() % GetTotalTunnelPortsCount());
	for (size_t i=0; i<this->TunnelPorts.size(); i++)
	{
		int count = TunnelPorts[i].EndPort - TunnelPorts[i].StartPort + 1;
		if (newPortIndex<count)
		{
			return (u_short)(TunnelPorts[i].StartPort + newPortIndex);
		}
		newPortIndex -= count;
	}

	return 0;
}


void BarbaClientConfig::ParseGrabProtocols(BarbaClientConfigItem* item, LPCTSTR value)
{
	TCHAR buffer[1000];
	_tcscpy_s(buffer, value);

	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(buffer, _T(","), &currentPos);
		
	while (token!=NULL)
	{
		ProtocolPort protocol;
		if (BarbaUtils::GetProtocolAndPort(token, &protocol.Protocol, &protocol.Port))
		{
			item->GrabProtocols.push_back(protocol);
		}
		token = _tcstok_s(NULL, _T(","), &currentPos);
	}
}


BarbaClientConfigManager::BarbaClientConfigManager()
{
}


void BarbaClientConfigManager::LoadFolder(LPCTSTR folder)
{
	std::vector<std::tstring> files;
	BarbaUtils::FindFiles(folder, _T("*.ini"), &files);
	for (size_t i=0; i<files.size(); i++)
	{
		BarbaClientConfig config;
		if (config.LoadFile(files[i].data()))
			Configs.push_back(config);
	}
}

BarbaClientConfig* BarbaClientConfigManager::FindByServerIP(DWORD ip)
{
	for (size_t i=0; i<this->Configs.size(); i++)
		if (this->Configs[i].ServerIp==ip)
			return &this->Configs[i];
	return NULL;
}

BarbaClientConfig::BarbaClientConfig()
{
	ServerIp = 0;
	ServerName[0] = 0;
}

bool BarbaClientConfig::LoadFile(LPCTSTR file)
{
	TCHAR serverAddress[255];
	GetPrivateProfileString(_T("General"), _T("ServerAddress"), _T(""), serverAddress, _countof(serverAddress), file);
	this->ServerIp = PacketHelper::ConvertStringIp(serverAddress);
	if (this->ServerIp==0)
		return false;

	//Name
	GetPrivateProfileString(_T("General"), _T("ServerName"), _T(""), ServerName, _countof(ServerName), file);

	//load Items
	int notfoundCounter = 0;
	for (int i=0; notfoundCounter<4; i++)
	{
		//create section name [Item1, Item2, ....]
		TCHAR sectionName[50];
		_stprintf_s(sectionName, _T("Item%d"), i+1);

		//read item
		BarbaClientConfigItem item;
		bool load = item.Load(sectionName, file);
		if (!load)
		{
			notfoundCounter++;
			continue;
		}

		//Name
		TCHAR grabProtocols[1000];
		GetPrivateProfileString(sectionName, _T("GrabProtocols"), _T(""), grabProtocols, _countof(grabProtocols), file);
		ParseGrabProtocols(&item, grabProtocols);
		this->Items.push_back(item);
	}

	return true;
}
