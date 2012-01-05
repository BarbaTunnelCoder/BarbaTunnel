#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"

BarbaServerApp* theServerApp = NULL;

BarbaServerApp::BarbaServerApp(void)
{
}


BarbaServerApp::~BarbaServerApp(void)
{
	if (!this->IsDisposed())
		Dispose();
}

void BarbaServerApp::Initialize()
{
	if (theServerApp!=NULL)
	{
		throw new BarbaException(_T("BarbaServerApp Already Initialized!"));
	}
	theServerApp = this;
	BarbaApp::Initialize();

	//configFileName
	TCHAR configFileName[MAX_PATH] = {0};
	GetPrivateProfileString(_T("Server"), _T("ConfigFileName"), _T(""), configFileName, _countof(configFileName), GetConfigFile());
	this->ConfigFileName = configFileName;

	//find config file
	TCHAR  configFile[MAX_PATH];
	if (this->ConfigFileName.empty())
	{
		std::vector<std::tstring> files;
		BarbaUtils::FindFiles( GetConfigItemFolder(), _T("*.ini"),  &files);
		if (files.size()==0) throw new BarbaException(_T("Could not find any config file."));
		else if (files.size()>1) throw new BarbaException(_T("BarbaServer find more than one config file! A ConfigFileName should specified in BarbaTunnel.ini."));
		else _tcscpy_s(configFile, files.front().data());
	}
	else
	{
		_stprintf_s(configFile, _T("%s\\%s"), GetConfigItemFolder(), configFileName);
	}

	//Load Config File
	if (!Config.LoadFile(configFile))
		throw new BarbaException(_T("Could not load %s file!"), configFileName);
	
	//load fake files
	TCHAR file[MAX_PATH];
	_stprintf_s(file, _T("%s\\templates\\HTTP-GetReplyTemplate.txt"), GetModuleFolder());
	this->FakeHttpGetReplyTemplate = BarbaUtils::LoadFileToString(file);
	_stprintf_s(file, _T("%s\\templates\\HTTP-PostReplyTemplate.txt"), GetModuleFolder());
	this->FakeHttpPostReplyTemplate = BarbaUtils::LoadFileToString(file);

	//AutoStartDelay
	this->AutoStartDelay = GetPrivateProfileInt(_T("Server"), _T("AutoStartDelay"), 0, GetConfigFile());

	//VirtualIpRange
	TCHAR virtualIpRange[100] = {0};
	GetPrivateProfileString(_T("Server"), _T("VirtualIpRange"), _T("") , virtualIpRange, _countof(virtualIpRange), GetConfigFile());
	if (_tcslen(virtualIpRange)==0) _tcscpy_s(virtualIpRange, _T("10.207.0.1"));
	TCHAR* dash = _tcschr(virtualIpRange, '-');
	TCHAR ipBuffer[100];
	_tcsncpy_s(ipBuffer, _countof(ipBuffer), virtualIpRange, dash!=NULL ? dash-virtualIpRange : _tcslen(virtualIpRange));
	this->VirtualIpRange.StartIp = PacketHelper::ConvertStringIp(ipBuffer);
	this->VirtualIpRange.EndIp = htonl( ntohl(VirtualIpRange.StartIp) + 0xFFFE ); //default
	if (dash!=NULL)
	{
		_tcscpy_s(ipBuffer, _countof(ipBuffer), dash+1);
		this->VirtualIpRange.EndIp = PacketHelper::ConvertStringIp(ipBuffer);
	}

	//Initialize Connection Manager
	ConnectionManager.Initialize(&this->VirtualIpRange);
}

void BarbaServerApp::Start()
{
	//Initialize HttpHost
	HttpHost.Initialize();
}

BarbaServerConfigItem* BarbaServerApp::ShouldGrabPacket(PacketHelper* packet)
{
	for (size_t i=0; i<this->Config.Items.size(); i++)
	{
		BarbaServerConfigItem* item = &this->Config.Items[i];
		
		//check protocol
		if (item->GetTunnelProtocol()!=packet->ipHeader->ip_p)
			continue;

		//check port
		u_short port = packet->GetDesPort();
		for (size_t j=0; j<item->TunnelPorts.size(); j++)
		{
			if (port>=item->TunnelPorts[j].StartPort && port<=item->TunnelPorts[j].EndPort)
				return item;
		}
	}

	return NULL;
}

void BarbaServerApp::Dispose()
{
	this->HttpHost.Dispose();
	this->ConnectionManager.Dispose();
	BarbaApp::Dispose();
}

bool BarbaServerApp::ProcessPacket(PacketHelper* packet, bool send)
{
	if (!packet->IsIp())
		return false;

	//find an open connection to process packet
	BarbaServerConnection* connection = (BarbaServerConnection*)ConnectionManager.FindByPacketToProcess(packet);
	
	//create new connection if not found
	if (!send && connection==NULL)
	{
		BarbaServerConfigItem* item = ShouldGrabPacket(packet);
		if (item!=NULL)
			connection = ConnectionManager.CreateConnection(packet, item);
	}
	
	//process packet for connection
	if (connection!=NULL)
		return connection->ProcessPacket(packet, send);

	return false;
}
