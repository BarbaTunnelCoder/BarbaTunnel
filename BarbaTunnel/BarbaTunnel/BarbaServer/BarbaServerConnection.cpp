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

//This function called when the connection should process the current IP
//In receive mode it should check signature after decrypt
//@return true if it process the packet
bool BarbaServerConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
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

		//static int i =0;
		//printf("\n\nServer sending packet!: ");
		//printf("\n%d, sum: %d\n", i++, packet.ipHeader->ip_sum);
		//printf("\norgPacket len: %d, sport%d, dport:%d\n", packet.GetPacketLen(), packet.GetSrcPort(), packet.GetDesPort());
		//BarbaUtils::PrintIp(packet.GetDesIp());
		//printf("\n");


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
