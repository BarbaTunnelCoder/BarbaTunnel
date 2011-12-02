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
		if (IsGrabPacket(packet, item))
			return item;
	}
	return NULL;
}

bool BarbaClientApp::IsGrabPacket(PacketHelper* packet, BarbaClientConfigItem* configItem)
{
	//check RealPort for redirect modes
	if (configItem->Mode==BarbaModeTcpRedirect || configItem->Mode==BarbaModeUdpRedirect)
	{
		return packet->GetDesPort()==configItem->RealPort;
	}


	for (size_t j=0; j<configItem->GrabProtocolsCount; j++)
	{
		//check GrabProtocols for tunnel modes
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

	/* sniff test
	DWORD testIp = PacketHelper::ConvertStringIp("?.0.0.0");
	DWORD testIp2 = PacketHelper::ConvertStringIp("?.?.?.?");
	static int i = 0;
	bool grab = (packet.GetDesIp()==testIp && packet.GetDesPort()==80) || (packet.GetSrcIp()==testIp && packet.GetSrcPort()==80);
	grab |= (packet.GetDesIp()==testIp2 && packet.GetDesPort()==443) || (packet.GetSrcIp()==testIp2 && packet.GetSrcPort()==443);

	if (i<200 && packet.IsTcp() && grab)
	{
		BYTE a[MAX_ETHER_FRAME] = {0};
		memcpy(a, packet.GetTcpPayload(), min(packet.GetTcpPayloadLen(), 10));

		if (packet.GetTcpPayloadLen()>3 && strncmp((char*)packet.GetTcpPayload(), "HTTP/1.1", 4)==0)
			i=i;


		i++;
		u_char flags = packet.tcpHeader->th_flags;
		printf("%s, FIN:%d, SYN:%d, RST:%d, PSH:%d, ACK:%d, seq:%u, ack:%u Len:%d, Data:%s\n", 
			send ? "Send" : "Receive",
			(flags&TH_FIN)!=0, 
			(flags&TH_SYN)!=0, 
			(flags&TH_RST)!=0, 
			(flags&TH_PSH)!=0, 
			(flags&TH_ACK)!=0, 
			htonl(packet.tcpHeader->th_seq),
			htonl(packet.tcpHeader->th_ack),
			packet.GetTcpPayloadLen(),
			a);
	}
	*/

	if (send)
	{
		BarbaClientConfig* config = ConfigManager.FindByServerIP(packet.GetDesIp());
		BarbaClientConfigItem* configItem = config!=NULL ? IsGrabPacket(&packet, config) : NULL;
		if (configItem==NULL)
			return;

		//create new connection if not found
		//in Redirect mode, the connection should always match the client port, to permit reestablish the connection
		connection = ConnectionManager.FindByConfigItem(configItem, /*configItem->IsRedirectMode() ? packet.GetSrcPort() :*/ 0);
		if (connection==NULL)
			connection = ConnectionManager.CreateConnection(&packet, config, configItem);
	}
	else
	{
		if (!packet.IsTcp() && !packet.IsUdp())
			return;

		//find packet that come from tunnel
		connection = ConnectionManager.Find(packet.GetSrcIp(), packet.ipHeader->ip_p, packet.GetDesPort());
	}

	//process packet for connection
	if (connection!=NULL)
	{
		connection->ProcessPacket(packetBuffer);
	}
}

