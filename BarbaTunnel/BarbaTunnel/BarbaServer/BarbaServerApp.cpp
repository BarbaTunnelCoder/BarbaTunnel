#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"

BarbaServerApp* theServerApp = NULL;

BarbaServerApp::BarbaServerApp(bool delayStart)
{
	this->DelayStart = delayStart;
	theServerApp = this;
	TCHAR file[MAX_PATH];

	//Load Configs
	BarbaServerConfig::LoadFolder(GetConfigFolder(), &this->Configs);
	
	//load fake files
	_stprintf_s(file, _T("%s\\templates\\HTTP-GetReplyTemplate.txt"), GetAppFolder());
	this->FakeHttpGetReplyTemplate = BarbaUtils::LoadFileToString(file);
	_stprintf_s(file, _T("%s\\templates\\HTTP-PostReplyTemplate.txt"), GetAppFolder());
	this->FakeHttpPostReplyTemplate = BarbaUtils::LoadFileToString(file);

	//AutoStartDelay
	this->AutoStartDelay = GetPrivateProfileInt(_T("Server"), _T("AutoStartDelay"), 0, GetSettingsFile());

	//VirtualIpRange
	TCHAR virtualIpRange[100] = {0};
	GetPrivateProfileString(_T("Server"), _T("VirtualIpRange"), _T("") , virtualIpRange, _countof(virtualIpRange), GetSettingsFile());
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
}

BarbaServerApp::~BarbaServerApp(void)
{
	if (!this->IsDisposed())
		Dispose();
}

void BarbaServerApp::Initialize()
{
	BarbaApp::Initialize();
	ConnectionManager.Initialize(&this->VirtualIpRange);
}

void BarbaServerApp::Start()
{
	//wait for server
	if (this->DelayStart && theApp->IsServerMode())
	{
		DWORD delayMin = theServerApp->AutoStartDelay;
		theApp->Comm.SetStatus(_T("Waiting"));
		BarbaLog(_T("Barba Server is waiting for AutoStartDelay (%d minutes)."), delayMin);
		BarbaNotify(_T("Barba Server is waiting\r\nBarba Server is waiting for %d minutes."), delayMin);
		Sleep(delayMin * 60* 1000);
	}

	//Initialize HttpHost
	HttpHost.Start();

	//Start
	BarbaApp::Start();
}

bool BarbaServerApp::ShouldGrabPacket(PacketHelper* packet, BarbaServerConfig* config)
{
	if (config->ServerIp!=0 && packet->GetDesIp()!=config->ServerIp)
		return false;

	//check protocol
	if (config->GetTunnelProtocol()!=packet->ipHeader->ip_p)
		return false;

	//check port
	u_short port = packet->GetDesPort();
	for (size_t j=0; j<config->TunnelPorts.size(); j++)
	{
		if (port>=config->TunnelPorts[j].StartPort && port<=config->TunnelPorts[j].EndPort)
			return true;
	}

	return false;
}

BarbaServerConfig* BarbaServerApp::ShouldGrabPacket(PacketHelper* packet)
{
	for (size_t i=0; i<this->Configs.size(); i++)
	{
		BarbaServerConfig* item = &this->Configs[i];
		if (ShouldGrabPacket(packet, item))
			return item;
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
	//find an open connection to process packet
	BarbaServerConnection* connection = (BarbaServerConnection*)ConnectionManager.FindByPacketToProcess(packet);
	
	//create new connection if not found
	if (!send && connection==NULL)
	{
		BarbaServerConfig* item = ShouldGrabPacket(packet);
		if (item!=NULL)
			connection = ConnectionManager.CreateConnection(packet, item);
	}
	
	//process packet for connection
	if (connection!=NULL)
		return connection->ProcessPacket(packet, send);

	return false;
}
