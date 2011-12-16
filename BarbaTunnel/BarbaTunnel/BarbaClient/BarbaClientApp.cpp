#include "StdAfx.h"
#include "BarbaClientApp.h"

extern CNdisApi			api;
BarbaClientApp* theClientApp = NULL;

BarbaClientApp::BarbaClientApp()
{
}

void BarbaClientApp::Initialize()
{
	if (theClientApp!=NULL)
	{
		throw _T("BarbaClientApp Already Initialized!");
	}
	theClientApp = this;
	BarbaApp::Initialize();

	TCHAR moduleFolder[MAX_PATH];
	BarbaUtils::GetModuleFolder(moduleFolder);
	ConfigManager.LoadFolder(moduleFolder);
}



BarbaClientConfigItem* BarbaClientApp::IsGrabPacket(PacketHelper* packet, BarbaClientConfig* config)
{
	for (int i=0; i<config->ItemsCount; i++)
	{
		BarbaClientConfigItem* item = &config->Items[i];
		if (item->IsGrabPacket(packet))
			return item;
	}
	return NULL;
}

bool BarbaSendPacketToAdapter(PacketHelper* packet, PINTERMEDIATE_BUFFER bufTemplate);
void BarbaClientApp::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);
	BarbaClientConnection* connection = NULL;
	if (!packet.IsIp())
		return;

	//if (packet.IsTcp())
		//printf("mode: %s, win:%d\n", send ? "Send" : "Receive", htons( packet.tcpHeader->th_win));

	//return;

	//static bool init = false;
	//DWORD testIp = PacketHelper::ConvertStringIp("?.?.?.?");
	//if (/*!send &&*/ packet.IsTcp())// && (packet.GetSrcIp()==testIp || packet.GetDesIp()==testIp) )
	//{
	//	char* data = (char*)packet.GetTcpPayload();
	//	if (packet.GetTcpPayloadLen()>5 && (strnicmp(data, "POST ", 4)==0 || strnicmp(data, "GET ", 4)==0 || strnicmp(data, "HTTP ", 4)==0 ))
	//		//init =true;
	//	//if (packet.GetTcpPayloadLen()>5 && (strnicmp(data, "GET ", 4)==0 || strnicmp(data, "POST ", 4)==0))
	//	//if (init)
	//	{
	//		char buf[2000] = {0};
	//		strncpy(buf, data, packet.GetTcpPayloadLen());
	//		printf("************\n%s\n***********", buf);
	//	}
	//}
	//return;

	//if (packet.GetDesIp()==testIp && packet.GetSrcPort()!=24547 && 0)
	//{
	//	init = true;
	//	tcpConn.InitClient(packet.GetSrcIp(), 80, 24547, packetBuffer);
	//	u_short pp  = packet.GetSrcPort();
	//	packetBuffer->m_Length = 0;
	//	return;
	//}



	if (send)
	{
		//find in current connections
		connection = ConnectionManager.FindByPacketToProcess(&packet);

		//create new connection if not found
		if (connection==NULL)
		{
			BarbaClientConfig* config = ConfigManager.FindByServerIP(packet.GetDesIp());
			BarbaClientConfigItem* configItem = config!=NULL ? IsGrabPacket(&packet, config) : NULL;
			if (configItem!=NULL)
				connection = ConnectionManager.CreateConnection(&packet, config, configItem);
		}
	}
	else
	{
		//find packet that come from tunnel
		connection = ConnectionManager.FindByPacketToProcess(&packet);
	}

	//process packet for connection
	if (connection!=NULL)
	{
		connection->ProcessPacket(packetBuffer);
	}
}

