#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaCrypt.h"
extern CNdisApi			api;

BarbaClientConnection::BarbaClientConnection()
{
	LasNegotiationTime = 0;
	Config = NULL;
	ConfigItem = NULL;
}

bool BarbaClientConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer)
{
	BarbaCrypt::CryptUdp(barbaPacket);
	PacketHelper orgPacket(orgPacketBuffer, 0);
	orgPacket.SetEthHeader(barbaPacket->ethHeader);
	orgPacket.SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload());
	return true;
}

bool BarbaClientConnection::CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer)
{
	PacketHelper barbaPacket(barbaPacketBuffer, IPPROTO_UDP);
	barbaPacket.SetEthHeader(packet->ethHeader);
	barbaPacket.ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket.ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.ipHeader->ip_off = 0;
	barbaPacket.SetSrcIp(packet->GetSrcIp());
	barbaPacket.SetDesIp(this->Config->ServerIp);
	barbaPacket.SetSrcPort(this->ConfigItem->TunnelPort);
	barbaPacket.SetDesPort(this->ConfigItem->TunnelPort);
	barbaPacket.SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	BarbaCrypt::CryptUdp(&barbaPacket);
	return true;
}

//This function called when the connection should process the current IP
//In receive mode it should check signature after decrypt
//@return true if it process the packet
bool BarbaClientConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		//static int i = 0;
		//printf("\norgPacket len: %d, sport%d, dport:%d", packet.GetPacketLen(), packet.GetSrcPort(), packet.GetDesPort());
		//if (packet.GetDesPort()!=1723)
			//return false;
			
		//Create Barba packet
		BYTE barbaPacket[MAX_ETHER_FRAME];
		if (!CreateUdpBarbaPacket(&packet, barbaPacket))
			return false;
			
		//replace current packet with barba packet
		packet.SetEthPacket((ether_header_ptr)barbaPacket);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		return true;
	}
	else
	{
		//extract Barba packet
		BYTE orgPacketBuffer[MAX_ETHER_FRAME];
		if (!ExtractUdpBarbaPacket(&packet, orgPacketBuffer))
			return false;
		PacketHelper orgPacket(orgPacketBuffer);
	
		packet.SetEthPacket(orgPacket.ethHeader);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();

		static int i =0;
		//printf("\n%d, sum: %d", i++, packet.ipHeader->ip_sum);
		printf("\norgPacket len: %d, sport%d, dport:%d\n", packet.GetPacketLen(), packet.GetSrcPort(), packet.GetDesPort());
		//BarbaUtils::PrintIp(orgPacket.GetSrcIp());
		//printf(" -- ");
		//BarbaUtils::PrintIp(orgPacket.GetDesIp());

		//extract org packet from barba packet
		return true;
	}
}
