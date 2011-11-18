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
		if (!item->GetTunnelProtocol()==packet->ipHeader->ip_p)
			continue;

		//check port
		for (int j=0; j<item->ListenPortsCount; j++)
		{
			u_short port = packet->GetDesPort();
			if (port>=item->ListenPorts[i].StartPort && port<=item->ListenPorts[i].EndPort)
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

BarbaServerConnection* BarbaServerConnectionManager::CreateConnection(PacketHelper* packet, BarbaServerConfigItem* configItem)
{
	BarbaServerConnection* conn = new BarbaServerConnection();
	conn->ConfigItem = configItem;
	memcpy_s(conn->ClientEthAddress, ETH_ALEN, packet->ethHeader->h_source, ETH_ALEN);
	conn->ClientIp = packet->GetSrcIp(); 
	conn->ClientFakeIp = theServerApp->VirtualIpManager.GetNewIp();
	conn->ClientPort = packet->GetSrcPort();
	conn->ClientTunnelPort = packet->GetDesPort();
	Connections[ConnectionsCount++] = conn;
	return conn;
}

void BarbaServerConnectionManager::RemoveConnection(BarbaServerConnection* conn)
{
	for (size_t i=0; i<ConnectionsCount; i++)
	{
		if (Connections[i]==conn)
		{
			Connections[i]=Connections[ConnectionsCount-1];
			theServerApp->VirtualIpManager.ReleaseIp(conn->ClientFakeIp);
			delete conn;
			ConnectionsCount--;
			break;
		}
	}
}
