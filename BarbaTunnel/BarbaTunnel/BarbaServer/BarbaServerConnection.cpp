#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"
#include "BarbaCrypt.h"


bool BarbaServerConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer)
{
	BarbaCrypt::CryptUdp(barbaPacket);
	PacketHelper orgPacket(orgPacketBuffer, 0);
	orgPacket.SetEthHeader(barbaPacket->ethHeader);
	orgPacket.SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload());
	return true;
}

bool BarbaServerConnection::CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer)
{
	PacketHelper barbaPacket(barbaPacketBuffer, IPPROTO_UDP);
	barbaPacket.SetEthHeader(packet->ethHeader);
	barbaPacket.ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket.ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.ipHeader->ip_off = 0;
	barbaPacket.SetSrcIp(packet->GetSrcIp());
	barbaPacket.SetDesIp(this->ClientIp);
	barbaPacket.SetSrcPort(this->ClientTunnelPort);
	barbaPacket.SetDesPort( this->ClientPort );
	barbaPacket.SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	barbaPacket.SetDesEthAddress(this->ClientEthAddress);
	BarbaCrypt::CryptUdp(&barbaPacket);
	return true;
}

bool BarbaServerConnection::ProcessPacketRedirect(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		packet.SetSrcPort(this->ClientTunnelPort);
		packet.SetDesIp(this->ClientIp);
		BarbaCrypt::CryptPacket(&packet);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
	}
	else
	{
		BarbaCrypt::CryptPacket(&packet);
		packet.SetSrcIp(this->ClientFakeIp);
		packet.SetDesPort(this->ConfigItem->RealPort);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
	}

	return true;
}

bool BarbaServerConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	switch(ConfigItem->Mode){
	case BarbaModeTcpRedirect:
	case BarbaModeUdpRedirect:
		return ProcessPacketRedirect(packetBuffer);
	
	case BarbaModeUdpTunnel:
		return ProcessPacketUdpTunnel(packetBuffer);
	}

	return false;
}

//This function called when the connection should process the current IP
//In receive mode it should check signature after decrypt
//@return true if it process the packet
bool BarbaServerConnection::ProcessPacketUdpTunnel(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		//test
		//packet.SetDesIp(this->PeerLocalIp);
		//packet.RecalculateChecksum();
		//packet.SetDesEthAddress(this->PeerEthAddress);
		//return true;
			

		packet.SetDesIp(this->ClientLocalIp);

		//Create Barba packet
		BYTE barbaPacketBuffer[MAX_ETHER_FRAME];
		CreateUdpBarbaPacket(&packet, barbaPacketBuffer);
		PacketHelper barbaPacket(barbaPacketBuffer);

		packet.SetEthPacket(barbaPacket.ethHeader);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		return true;
	}
	else
	{
		//extract Barba packet
		BYTE orgPacketBuffer[MAX_ETHER_FRAME];
		//printf("barba tunnel\n");
		if (!ExtractUdpBarbaPacket(&packet, orgPacketBuffer))
			return false;
		PacketHelper orgPacket(orgPacketBuffer);

		//Init First Attempt
		if (this->ClientLocalIp==0)
		{
			this->ClientLocalIp = orgPacket.GetSrcIp();
		}
			
		//prepare for NAT
		orgPacket.SetSrcIp(this->ClientFakeIp);
		orgPacket.RecalculateChecksum();

		//printf("barba restored\n");

		//replace current packet with barba packet
		//static int i = 0;
		packet.SetEthPacket(orgPacket.ethHeader);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		return true;
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
	BarbaUtils::PrintIp(conn->ClientIp );
	printf(" - ");
	BarbaUtils::PrintIp(conn->ClientFakeIp);
	printf("\n");

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
