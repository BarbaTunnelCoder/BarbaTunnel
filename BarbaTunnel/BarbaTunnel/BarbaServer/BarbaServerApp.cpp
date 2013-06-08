#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"

BarbaServerApp* theServerApp = NULL;

BarbaServerApp::BarbaServerApp(bool delayStart)
{
	DelayStart = delayStart;
	theServerApp = this;
}

BarbaServerApp::~BarbaServerApp(void)
{
	if (!IsDisposed())
		Dispose();
}

void BarbaServerApp::Load()
{
	BarbaApp::Load();

	//load template files
	TCHAR file[MAX_PATH];
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request\\GetReply.txt"), GetAppFolder());
	HttpGetReplyTemplate = BarbaUtils::PrepareHttpRequest( BarbaUtils::LoadFileToString(file) );
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request\\PostReply.txt"), GetAppFolder());
	HttpPostReplyTemplate = BarbaUtils::PrepareHttpRequest(BarbaUtils::LoadFileToString(file));
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request-Bombard\\GetReply.txt"), GetAppFolder());
	HttpGetReplyTemplateBombard = BarbaUtils::PrepareHttpRequest(BarbaUtils::LoadFileToString(file));
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request-Bombard\\PostReply.txt"), GetAppFolder());
	HttpPostReplyTemplateBombard = BarbaUtils::PrepareHttpRequest(BarbaUtils::LoadFileToString(file));

	//AutoStartDelay
	AutoStartDelay = GetPrivateProfileInt(_T("Server"), _T("AutoStartDelay"), 0, GetSettingsFile());

	//VirtualIpRange
	TCHAR virtualIpRange[100] = {0};
	GetPrivateProfileString(_T("Server"), _T("VirtualIpRange"), _T("") , virtualIpRange, _countof(virtualIpRange), GetSettingsFile());
	if (_tcslen(virtualIpRange)==0) _tcscpy_s(virtualIpRange, _T("10.207.0.1"));
	TCHAR* dash = _tcschr(virtualIpRange, '-');
	TCHAR ipBuffer[100];
	_tcsncpy_s(ipBuffer, _countof(ipBuffer), virtualIpRange, dash!=NULL ? dash-virtualIpRange : _tcslen(virtualIpRange));
	VirtualIpRange.StartIp = inet_addr(ipBuffer);
	VirtualIpRange.EndIp = htonl( ntohl(VirtualIpRange.StartIp) + 0xFFFE ); //default
	if (dash!=NULL)
	{
		_tcscpy_s(ipBuffer, _countof(ipBuffer), dash+1);
		VirtualIpRange.EndIp = inet_addr(ipBuffer);
	}

	//Load Configs
	BarbaServerConfig::LoadFolder(GetConfigFolder(), &Configs);
}

void BarbaServerApp::Initialize()
{
	BarbaApp::Initialize();
	ConnectionManager.Initialize(&VirtualIpRange);
}

void BarbaServerApp::Start()
{
	//wait for server
	if (DelayStart && theApp->IsServerMode())
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

void BarbaServerApp::Dispose()
{
	HttpHost.Dispose();
	ConnectionManager.Dispose();
	BarbaApp::Dispose();
}

bool BarbaServerApp::IsConfigForInboundPacket(BarbaServerConfig* config, PacketHelper* packet)
{
	if (config->ServerIp!=0 && packet->GetDesIp()!=config->ServerIp)
		return false;

	return config->GetTunnelProtocol()==packet->ipHeader->ip_p && config->TunnelPorts.IsPortInRange(packet->GetDesPort());
}

BarbaServerConfig* BarbaServerApp::FindConfigForInboundPacket(PacketHelper* packet)
{
	for (size_t i=0; i<Configs.size(); i++)
		if (IsConfigForInboundPacket(&Configs[i], packet))
			return &Configs[i];
	return NULL;
}

bool BarbaServerApp::ProcessInboundPacket(PacketHelper* packet)
{
	BarbaServerConfig* config = FindConfigForInboundPacket(packet);
	if (config==NULL)
		return false;

	//find existing connection who can process packet
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&ConnectionManager.Connections);
	BarbaServerConnection** connections = (BarbaServerConnection**)autoLockBuf.GetBuffer();
	for (size_t i=0; i<ConnectionManager.Connections.GetCount(); i++)
	{
		BarbaServerConnection* connection = connections[i];
		if ( connection->GetConfig()==config && connection->GetClientIp()==packet->GetSrcIp() && connection->ProcessInboundPacket(packet) )
			return true;
	}

	//create connection if not proceed
	BarbaConnection* connection = ConnectionManager.CreateConnection(config, packet);
	return connection->ProcessInboundPacket(packet);	
}

bool BarbaServerApp::ProcessOutboundPacket(PacketHelper* packet)
{
	BarbaServerConnection* connection = (BarbaServerConnection*)ConnectionManager.FindByVirtualIp(packet->GetDesIp());
	if (connection==NULL)
		return false;

	connection->ProcessOutboundPacket(packet);
	return true;
}
