#include "StdAfx.h"
#include "BarbaApp.h"

extern CNdisApi			api;
BarbaApp* theApp = NULL;

// {E0B6D3F2-5B74-46E7-BC2C-5C349055DD06}
GUID BarbaSign = { 0xe0b6d3f2, 0x5b74, 0x46e7, { 0xbc, 0x2c, 0x5c, 0x34, 0x90, 0x55, 0xdd, 0x6 } };

GUID* BarbaApp::GetBarbaSign()
{
	return &BarbaSign;
}

BarbaApp::BarbaApp()
{
	ZeroMemory ( &CurrentRequest, sizeof(ETH_REQUEST) );
	ZeroMemory ( &PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	CurrentRequest.EthPacket.Buffer = &PacketBuffer;
	IsDebugMode = false;
	IpInc = 0;
}

void BarbaApp::Init()
{
	if (theApp!=NULL)
	{
		throw _T("BarbaApp Already Initialized!");
	}
	theApp = this;

	TCHAR moduleFolder[MAX_PATH];
	BarbaUtils::GetModuleFolder(moduleFolder);
	ConfigManager.LoadFolder(moduleFolder);
}


bool BarbaApp::IsServer()
{
	return ConfigManager.GetServerConfig()!=NULL;
}

bool BarbaApp::CheckTerminateCommands(INTERMEDIATE_BUFFER* packet)
{
	bool send = packet->m_dwDeviceFlags == PACKET_FLAG_ON_SEND;
	ether_header_ptr ethHeader = (ether_header*)packet->m_IBuffer;
	if (send || ntohs(ethHeader->h_proto)!=ETH_P_IP)
		return false;

	iphdr_ptr ipHeader = (iphdr*)(ethHeader + 1);
	if (ipHeader->ip_p!=1)
		return false;

	int nlen = ntohs(ipHeader->ip_len);
	int code = nlen - 28;
	if (code==1350)
			return true;

	return false;
}

//return true if the packed should grabbed before send
bool BarbaApp::IsClientGrabPacket(PacketHelper* packet, BarbaConfig* config)
{
	for (int i=0; i<config->GrabProtocolsCount; i++)
	{
		ProtocolPort* protocolPort = &config->GrabProtocols[i];
		if (protocolPort->Protocol==0 || protocolPort->Protocol==packet->ipHeader->ip_p)
		{
			if (protocolPort->Port==0 || protocolPort->Port==packet->GetDesPort())
				return true;
		}

	}
	return false;
}

void BarbaApp::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);
	BarbaConnection* connection = NULL;
	if (!packet.IsIp())
		return;

	//client mode
	if (IsServer())
	{
		BarbaConfig* serverConfig = ConfigManager.GetServerConfig();
		if (send)
		{
			connection = ConnectionManager.FindByFakeIp(packet.GetDesIp());
		}
		else
		{
			//fast ignore if incoming packet in server does not came from tunnel
			//if (packet.IsUdp())
				//printf("udp get! dport:%d\n", packet.GetDesPort());
			if (packet.ipHeader->ip_p!=serverConfig->TunnelProtocol.Protocol || packet.GetDesPort()!=serverConfig->TunnelProtocol.Port)
				return;

			//find or create connection
			connection = ConnectionManager.FindByIp(packet.GetSrcIp(), packet.GetSrcPort());
			if (connection==NULL)
				connection = ConnectionManager.CreateConnection(packet.GetSrcIp(), packet.GetSrcPort(), serverConfig);
		}
	}
	else
	{
		if (send)
		{
			connection = ConnectionManager.FindByIp(packet.GetDesIp(), 0);
			if (connection==NULL)
			{
				BarbaConfig* config = ConfigManager.FindByServerIP(packet.GetDesIp());
				if (config!=NULL && IsClientGrabPacket(&packet, config))
					connection = ConnectionManager.CreateConnection(packet.GetDesIp(), 0, config);
			}

			//ignore if packet should go to tunnel
			if (connection!=NULL && !IsClientGrabPacket(&packet, connection->Config))
				return;
		}
		else
		{
			if (!packet.IsTcp() && !packet.IsUdp())
				return;

			//make sure packet came from tunnel
			connection = ConnectionManager.FindByIp(packet.GetSrcIp(), 0);

			//fast ignore if incoming packet in server does not came from tunnel
			if (connection!=NULL && (packet.ipHeader->ip_p!=connection->Config->TunnelProtocol.Protocol || packet.GetSrcPort()!=connection->Config->TunnelProtocol.Port))
				return;
		}
	}

	//process packet for connection
	if (connection!=NULL)
	{
		connection->ProcessPacket(packetBuffer);
	}
}

