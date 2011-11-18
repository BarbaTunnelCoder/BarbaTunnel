#include "StdAfx.h"
#include "BarbaClientApp.h"

extern CNdisApi			api;
BarbaClientApp* theClientApp = NULL;

BarbaClientApp::BarbaClientApp()
{
}

void BarbaClientApp::Init()
{
	if (theClientApp!=NULL)
	{
		throw _T("BarbaClientApp Already Initialized!");
	}
	theClientApp = this;

	TCHAR moduleFolder[MAX_PATH];
	BarbaUtils::GetModuleFolder(moduleFolder);
	ConfigManager.LoadFolder(moduleFolder);
}



BarbaClientConfigItem* BarbaClientApp::IsGrabPacket(PacketHelper* packet, BarbaClientConfig* config)
{
	for (int i=0; i<config->ItemsCount; i++)
	{
		BarbaClientConfigItem* item = &config->Items[i];
		if (IsGrabPacket(packet, item))
			return item;
	}
	return NULL;
}

bool BarbaClientApp::IsGrabPacket(PacketHelper* packet, BarbaClientConfigItem* configItem)
{
	for (int j=0; j<configItem->GrabProtocolsCount; j++)
	{
		ProtocolPort* protocolPort = &configItem->GrabProtocols[j];
		if (protocolPort->Protocol==0 || protocolPort->Protocol==packet->ipHeader->ip_p)
		{
			if (protocolPort->Port==0 || protocolPort->Port==packet->GetDesPort())
				return true;
		}
	}

	return false;
}


void BarbaClientApp::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);
	BarbaClientConnection* connection = NULL;
	if (!packet.IsIp())
		return;

	if (send)
	{
		BarbaClientConfig* config = ConfigManager.FindByServerIP(packet.GetDesIp());
		BarbaClientConfigItem* configItem = config!=NULL ? IsGrabPacket(&packet, config) : NULL;
		if (configItem==NULL)
			return;

		//create new connection if not found
		connection = ConnectionManager.FindByConfigItem(configItem);
		if (connection==NULL)
			connection = ConnectionManager.CreateConnection(config, configItem);
	}
	else
	{
		if (!packet.IsTcp() && !packet.IsUdp())
			return;

		//make sure packet came from tunnel
		connection = ConnectionManager.Find(packet.GetSrcIp(), packet.ipHeader->ip_p, packet.GetDesPort());
	}

	//process packet for connection
	if (connection!=NULL)
	{
		connection->ProcessPacket(packetBuffer);
	}
}

