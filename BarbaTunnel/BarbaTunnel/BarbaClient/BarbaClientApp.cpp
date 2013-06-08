#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaClientApp.h"

BarbaClientApp* theClientApp = NULL;
BarbaClientApp::BarbaClientApp()
{
	theClientApp = this;
}

BarbaClientApp::~BarbaClientApp()
{
	if (!IsDisposed())
		Dispose();
}

void BarbaClientApp::Dispose()
{
	ConnectionManager.Dispose();
	BarbaApp::Dispose();
}

void BarbaClientApp::Load()
{
	BarbaApp::Load();
	TCHAR file[MAX_PATH];
	
	//load template files
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request\\Get.txt"), GetAppFolder());
	HttpGetTemplate = BarbaUtils::PrepareHttpRequest( BarbaUtils::LoadFileToString(file) );
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request\\Post.txt"), GetAppFolder());
	HttpPostTemplate = BarbaUtils::PrepareHttpRequest(BarbaUtils::LoadFileToString(file));
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request-Bombard\\Get.txt"), GetAppFolder());
	HttpGetTemplateBombard = BarbaUtils::PrepareHttpRequest(BarbaUtils::LoadFileToString(file));
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request-Bombard\\Post.txt"), GetAppFolder());
	HttpPostTemplateBombard = BarbaUtils::PrepareHttpRequest(BarbaUtils::LoadFileToString(file));

	//Load Configs
	BarbaClientConfig::LoadFolder(GetConfigFolder(), &Configs);
}

void BarbaClientApp::Initialize()
{
	BarbaApp::Initialize();
}


bool BarbaClientApp::IsConfigForOutboundPacket(BarbaClientConfig* config, PacketHelper* packet)
{
	if (config->ServerIp!=packet->GetDesIp())
		return false;

	//check RealPort for redirect modes
	if (config->Mode==BarbaModeTcpRedirect || config->Mode==BarbaModeUdpRedirect)
		return packet->GetDesPort()==config->RealPort;

	for (size_t i=0; i<config->GrabProtocols.size(); i++)
	{
		//check GrabProtocols for tunnel modes
		ProtocolPort* protocolPort = &config->GrabProtocols[i];
		if (protocolPort->Protocol==0 || protocolPort->Protocol==packet->ipHeader->ip_p)
		{
			if (protocolPort->Port==0 || protocolPort->Port==packet->GetDesPort())
				return true;
		}
	}

	return false;
}

bool BarbaClientApp::IsConfigForInboundPacket(BarbaClientConfig* config, PacketHelper* packet)
{
	return 
		config->ServerIp == packet->GetSrcIp() &&
		config->GetTunnelProtocol() == packet->ipHeader->ip_p &&
		config->TunnelPorts.IsPortInRange(packet->GetSrcPort());
}

BarbaClientConfig* BarbaClientApp::FindConfigForOutboundPacket(PacketHelper* packet)
{
	for (size_t i=0; i<Configs.size(); i++)
		if (IsConfigForOutboundPacket(&Configs[i], packet))
			return &Configs[i];
	return NULL;
}

BarbaClientConfig* BarbaClientApp::FindConfigForInboundPacket(PacketHelper* packet)
{
	for (size_t i=0; i<Configs.size(); i++)
		if (IsConfigForInboundPacket(&Configs[i], packet))
			return &Configs[i];
	return NULL;
}

bool TestPacket(PacketHelper* packet, bool send) 
{
	UNREFERENCED_PARAMETER(send);
	UNREFERENCED_PARAMETER(packet);
	return false;
}

bool BarbaClientApp::ProcessOutboundPacket(PacketHelper* packet)
{
	//just for debug
	if (TestPacket(packet, true)) 
		return true;

	//find config by grab protocol
	BarbaClientConfig* config = FindConfigForOutboundPacket(packet);
	if (config==NULL)
		return false;

	//find existing connection who can process packet
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&ConnectionManager.Connections);
	BarbaClientConnection** connections = (BarbaClientConnection**)autoLockBuf.GetBuffer();
	for (size_t i=0; i<ConnectionManager.Connections.GetCount(); i++)
	{
		BarbaClientConnection* connection = connections[i];
		if ( connection->GetConfig()==config && connection->ProcessOutboundPacket(packet) )
			return true;
	}

	//create new connection
	BarbaClientConnection* connection = ConnectionManager.CreateConnection(packet, config);
	return connection->ProcessOutboundPacket(packet);	
}

bool BarbaClientApp::ProcessInboundPacket(PacketHelper* packet)
{
	//just for debug
	if (TestPacket(packet, false)) 
		return true;

	//find config by grab protocol
	BarbaClientConfig* config = FindConfigForInboundPacket(packet);
	if (config==NULL)
		return false;

	//find existing connection who can process packet
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&ConnectionManager.Connections);
	BarbaClientConnection** connections = (BarbaClientConnection**)autoLockBuf.GetBuffer();
	for (size_t i=0; i<ConnectionManager.Connections.GetCount(); i++)
	{
		BarbaClientConnection* connection = connections[i];
		if ( connection->GetConfig()==config && connection->ProcessInboundPacket(packet) )
			return true;
	}

	return false;	
}
