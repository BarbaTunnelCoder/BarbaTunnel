#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"

BarbaServerApp* theServerApp = NULL;

BarbaServerApp::BarbaServerApp(void)
{
	VirtualIpInc = 0;
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
		//fast ignore if incoming packet in server does not came from tunnel
		//if (packet.IsUdp())
			//printf("udp get! dport:%d\n", packet.GetDesPort());
		//if (packet.ipHeader->ip_p!=Config.TunnelProtocol || packet.GetDesPort()!=serverConfig->TunnelProtocol.Port)
		//	return;

		////find or create connection
		//connection = ConnectionManager.FindByIp(packet.GetSrcIp(), packet.GetSrcPort());
		//if (connection==NULL)
		//	connection = ConnectionManager.CreateConnection(packet.GetSrcIp(), packet.GetSrcPort(), serverConfig);
	}

	//process packet for connection
	if (connection!=NULL)
	{
		connection->ProcessPacket(packetBuffer);
	}
}
