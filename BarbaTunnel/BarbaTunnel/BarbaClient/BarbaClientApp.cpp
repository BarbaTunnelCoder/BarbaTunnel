#include "StdAfx.h"
#include "BarbaClientApp.h"

BarbaClientApp* theClientApp = NULL;
BarbaClientApp::BarbaClientApp()
{
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
	if (theClientApp!=NULL)
	{
		throw new BarbaException(_T("BarbaClientApp Already Initialized!"));
	}
	theClientApp = this;
	BarbaApp::Initialize();

	TCHAR file[MAX_PATH];
	_stprintf_s(file, _countof(file), _T("%s\\config"), GetModuleFolder());
	ConfigManager.LoadFolder(file);

	//load fake files
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-GetTemplate.txt"), GetModuleFolder());
	this->FakeHttpGetTemplate = BarbaUtils::LoadFileToString(file);
	_stprintf_s(file, _countof(file), _T("%s\\templates\\HTTP-PostTemplate.txt"), GetModuleFolder());
	this->FakeHttpPostTemplate = BarbaUtils::LoadFileToString(file);

}



BarbaClientConfigItem* BarbaClientApp::ShouldGrabPacket(PacketHelper* packet, BarbaClientConfig* config)
{
	for (int i=0; i<(int)config->Items.size(); i++)
	{
		BarbaClientConfigItem* item = &config->Items[i];
		if (item->ShouldGrabPacket(packet))
			return item;
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
	if (!packet->IsIp())
		return false;

	//just for debug
	if (TestPacket(packet, send))
		return true;

	//find an open connection to process packet
	BarbaClientConnection* connection = (BarbaClientConnection*)ConnectionManager.FindByPacketToProcess(packet);
	
	//create new connection if not found
	if (send && connection==NULL)
	{
		BarbaClientConfig* config = ConfigManager.FindByServerIP(packet->GetDesIp());
		BarbaClientConfigItem* configItem = config!=NULL ? ShouldGrabPacket(packet, config) : NULL;
		if (configItem!=NULL)
			connection = ConnectionManager.CreateConnection(packet, config, configItem);
	}

	//process packet for connection
	if (connection!=NULL)
		return connection->ProcessPacket(packet, send);

	return false;
}

