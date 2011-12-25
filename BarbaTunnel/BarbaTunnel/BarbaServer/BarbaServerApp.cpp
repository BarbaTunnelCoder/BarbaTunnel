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

	TCHAR moduleFolder[MAX_PATH];
	BarbaUtils::GetModuleFolder(moduleFolder);

	//load server.ini
	TCHAR file[MAX_PATH];
	_stprintf_s(file, _countof(file), _T("%s\\server\\config\\server.ini"), moduleFolder);
	Config.LoadFile(file);

	//load fake files
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-GetReplyTemplate.txt"), moduleFolder);
	this->FakeHttpGetReplyTemplate = BarbaUtils::LoadFileToString(file);
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-PostReplyTemplate.txt"), moduleFolder);
	this->FakeHttpPostReplyTemplate = BarbaUtils::LoadFileToString(file);

	//Initialize Connection Manager
	ConnectionManager.Initialize(&Config.VirtualIpRange);

}

void BarbaServerApp::Start()
{
	//Initialize HttpHost
	HttpHost.Initialize();
}

BarbaServerConfigItem* BarbaServerApp::ShouldGrabPacket(PacketHelper* packet)
{
	for (size_t i=0; i<this->Config.ItemsCount; i++)
	{
		BarbaServerConfigItem* item = &this->Config.Items[i];
		
		//check protocol
		if (item->GetTunnelProtocol()!=packet->ipHeader->ip_p)
			continue;

		//check port
		u_short port = packet->GetDesPort();
		for (size_t j=0; j<item->TunnelPortsCount; j++)
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
