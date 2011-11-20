#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"

BarbaServerApp* theServerApp = NULL;

BarbaServerApp::BarbaServerApp(void)
	: VirtualIpManager(&Config.VirtualIpRange)
{
}


BarbaServerApp::~BarbaServerApp(void)
{
}

void BarbaServerApp::Init()
{
	if (theServerApp!=NULL)
	{
		throw _T("BarbaServerApp Already Initialized!");
	}
	theServerApp = this;

	TCHAR moduleFolder[MAX_PATH];
	BarbaUtils::GetModuleFolder(moduleFolder);

	TCHAR file[MAX_PATH];
	_stprintf_s(file, _countof(file), _T("%s\\config\\server.cfg"), moduleFolder);
	Config.LoadFile(file);
}

BarbaServerConfigItem* BarbaServerApp::IsGrabPacket(PacketHelper* packet)
{
	for (int i=0; i<this->Config.ItemsCount; i++)
	{
		BarbaServerConfigItem* item = &this->Config.Items[i];
		//check protocol
		if (item->GetTunnelProtocol()!=packet->ipHeader->ip_p)
			continue;

		//check port
		u_short port = packet->GetDesPort();
		for (int j=0; j<item->ListenPortsCount; j++)
		{
			if (port>=item->ListenPorts[j].StartPort && port<=item->ListenPorts[j].EndPort)
				return item;
		}
	}

	return NULL;
}

void BarbaServerApp::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);
	BarbaServerConnection* connection = NULL;
	if (!packet.IsIp())
		return;

	if (send)
	{
		connection = ConnectionManager.FindByFakeIp(packet.GetDesIp());
	}
	else
	{
		BarbaServerConfigItem* item = IsGrabPacket(&packet);
		if (item==NULL)
			return;
		
		//find or create connection
		connection = ConnectionManager.Find(packet.GetSrcIp(), packet.GetSrcPort(), item);
		if (connection==NULL)
			connection = ConnectionManager.CreateConnection(&packet, item);
	}

	//process packet for connection
	if (connection!=NULL)
	{
		connection->ProcessPacket(packetBuffer);
	}
}
