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

	TCHAR file[MAX_PATH];
	_stprintf_s(file, _countof(file), _T("%s\\client\\config"), GetModuleFolder());
	ConfigManager.LoadFolder(file);

	//load fake files
	_stprintf_s(file, _countof(file), _T("%s\\client\\templates\\HTTP-GetTemplate.txt"), GetModuleFolder());
	this->FakeHttpGetTemplate = BarbaUtils::LoadFileToString(file);
	_stprintf_s(file, _countof(file), _T("%s\\client\\templates\\HTTP-PostTemplate.txt"), GetModuleFolder());
	this->FakeHttpPostTemplate = BarbaUtils::LoadFileToString(file);

}



BarbaClientConfigItem* BarbaClientApp::ShouldGrabPacket(PacketHelper* packet, BarbaClientConfig* config)
{
	for (int i=0; i<config->ItemsCount; i++)
	{
		BarbaClientConfigItem* item = &config->Items[i];
		if (item->ShouldGrabPacket(packet))
			return item;
	}
	return NULL;
}

//for debug
bool TestPacket(PacketHelper* packet, bool send) 
{
	UNREFERENCED_PARAMETER(send);
	UNREFERENCED_PARAMETER(packet);
	
	//static bool init = false;
	//DWORD testIp = PacketHelper::ConvertStringIp("0.0.0.0");
	//testIp = PacketHelper::ConvertStringIp("0.0.0.0");
	//if (/*!send &&*/ packet->IsTcp() && (packet->GetSrcIp()==testIp || packet->GetDesIp()==testIp) )
	//{
	//	char* data = (char*)packet->GetTcpPayload();
	//	if (packet->GetTcpPayloadLen()>5 && (strnicmp(data, "POST ", 4)==0 || strnicmp(data, "GET ", 4)==0 || strnicmp(data, "HTTP ", 4)==0 ))
	//		//init =true;
	//	{
	//		char buf[2000] = {0};
	//		strncpy(buf, data, packet->GetTcpPayloadLen());
	//		printf("************\n%s\n***********", buf);
	//	}
	//}

	//sniff test
	//DWORD testIp = PacketHelper::ConvertStringIp("192.168.0.23");
	//static int i = 0;
	//bool grab = /*(packet->GetDesIp()==testIp && packet->GetDesPort()==80) ||*/ (packet->GetSrcIp()==testIp && packet->GetSrcPort()==8080);
	//if (i<200 && packet->IsTcp() && grab)
	//{
	//	static u_short port = packet->GetDesPort();
	//	if (port==packet->GetDesPort())
	//	{

	//	BYTE a[MAX_ETHER_FRAME] = {0};
	//	i++;
	//	u_char flags = packet->tcpHeader->th_flags;
	//	printf("port:%d, %s, FIN:%d, SYN:%d, RST:%d, PSH:%d, ACK:%d, seq:%u, ack:%u Len:%d, Data:%s\n", packet->GetDesPort(),
	//		send ? "Send" : "Receive",
	//		(flags&TH_FIN)!=0, 
	//		(flags&TH_SYN)!=0, 
	//		(flags&TH_RST)!=0, 
	//		(flags&TH_PSH)!=0, 
	//		(flags&TH_ACK)!=0, 
	//		htonl(packet->tcpHeader->th_seq),
	//		htonl(packet->tcpHeader->th_ack),
	//		packet->GetTcpPayloadLen(),
	//		a);
	//	}
	//}

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

