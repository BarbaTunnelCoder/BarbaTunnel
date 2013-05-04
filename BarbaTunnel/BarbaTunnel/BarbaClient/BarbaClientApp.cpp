#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaClientApp.h"

BarbaClientApp* theClientApp = NULL;
BarbaClientApp::BarbaClientApp()
{
	theClientApp = this;
	TCHAR file[MAX_PATH];
	
	//Load Configs
	BarbaClientConfig::LoadFolder(GetConfigFolder(), &this->Configs);

	//load template files
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request\\Get.txt"), GetAppFolder());
	this->HttpGetTemplate = BarbaUtils::PrepareHttpRequest( BarbaUtils::LoadFileToString(file) );
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request\\Post.txt"), GetAppFolder());
	this->HttpPostTemplate = BarbaUtils::PrepareHttpRequest(BarbaUtils::LoadFileToString(file));
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request-PerPacket\\Get.txt"), GetAppFolder());
	this->HttpGetTemplatePerPacket = BarbaUtils::PrepareHttpRequest(BarbaUtils::LoadFileToString(file));
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-Request-PerPacket\\Post.txt"), GetAppFolder());
	this->HttpPostTemplatePerPacket = BarbaUtils::PrepareHttpRequest(BarbaUtils::LoadFileToString(file));
}

BarbaClientApp::~BarbaClientApp()
{
	if (!this->IsDisposed())
		Dispose();
}

void BarbaClientApp::Dispose()
{
	this->ConnectionManager.Dispose();
	BarbaApp::Dispose();
}

void BarbaClientApp::Initialize()
{
	BarbaApp::Initialize();
}


bool BarbaClientApp::ShouldGrabPacket(PacketHelper* packet, BarbaClientConfig* config)
{
	if (packet->GetDesIp()!=config->ServerIp)
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

BarbaClientConfig* BarbaClientApp::ShouldGrabPacket(PacketHelper* packet)
{
	for (size_t i=0; i<this->Configs.size(); i++)
	{
		BarbaClientConfig* config = &this->Configs[i];
		if (ShouldGrabPacket(packet, config))
			return config;
	}
	return NULL;
}

bool TestPacket(PacketHelper* packet, bool send) 
{
	UNREFERENCED_PARAMETER(send);
	UNREFERENCED_PARAMETER(packet);
	return false;
}

bool BarbaClientApp::ProcessPacket(PacketHelper* packet, bool send)
{
	//just for debug
	if (TestPacket(packet, send))
		return true;

	//find an open connection to process packet
	BarbaClientConnection* connection = (BarbaClientConnection*)ConnectionManager.FindByPacketToProcess(packet);
	
	//create new connection if not found
	if (send && connection==NULL)
	{
		BarbaClientConfig* config = ShouldGrabPacket(packet);
		if (config!=NULL)
			connection = ConnectionManager.CreateConnection(packet, config);
	}

	//process packet for connection
	if (connection!=NULL)
		return connection->ProcessPacket(packet, send);

	return false;
}
