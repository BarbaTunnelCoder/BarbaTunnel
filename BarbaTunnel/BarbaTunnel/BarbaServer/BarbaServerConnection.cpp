#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"


BarbaServerConnection::BarbaServerConnection(void)
{
}


BarbaServerConnection::~BarbaServerConnection(void)
{
}

bool BarbaServerConnection::ProcessPacket(INTERMEDIATE_BUFFER* /*packet*/)
{
	return true;
}

/*
size_t BarbaClientConnection::CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer)
{
	PacketHelper barbaPacket(barbaPacketBuffer, IPPROTO_UDP);
	barbaPacket.SetEthHeader(packet->ethHeader);
	barbaPacket.ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket.ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.ipHeader->ip_off = 0;
	barbaPacket.SetSrcIp(packet->GetSrcIp());
	barbaPacket.SetDesIp(this->PeerIP);
	barbaPacket.SetSrcPort(this->Config->TunnelProtocol.Port);
	barbaPacket.SetDesPort( theApp->IsServer() ? this->PeerPort : this->Config->TunnelProtocol.Port);
	barbaPacket.SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	if (theApp->IsServer()) barbaPacket.SetDesEthAddress(this->PeerEthAddress);
	return barbaPacket.GetPacketLen();
}

//This function called when the connection should process the current IP
//In receive mode it should check signature after decrypt
//@return true if it process the packet
bool BarbaClientConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (theApp->IsServer())
	{
		if (send)
		{
			//test
			//packet.SetDesIp(this->PeerLocalIp);
			//packet.RecalculateChecksum();
			//packet.SetDesEthAddress(this->PeerEthAddress);
			//return true;
			

			packet.SetDesIp(this->PeerLocalIp);

			//static int i =0;
			//printf("\n\nServer sending packet!: ");
			//printf("\n%d, sum: %d\n", i++, packet.ipHeader->ip_sum);
			//printf("\norgPacket len: %d, sport%d, dport:%d\n", packet.GetPacketLen(), packet.GetSrcPort(), packet.GetDesPort());
			//BarbaUtils::PrintIp(packet.GetDesIp());
			//printf("\n");


			//Create Barba packet
			BYTE barbaPacketBuffer[MAX_ETHER_FRAME];
			size_t length = CreateUdpBarbaPacket(&packet, barbaPacketBuffer);
			PacketHelper barbaPacket(barbaPacketBuffer);

			packet.SetEthPacket(barbaPacket.ethHeader);
			CryptUdp(packet.ethHeader);
			packet.RecalculateChecksum();
			packetBuffer->m_Length = packet.GetPacketLen();
			return true;
		}
		else
		{
			//extract Barba packet
			BYTE orgPacketBuffer[MAX_ETHER_FRAME];
			//printf("barba tunnel\n");
			CryptUdp(packet.ethHeader);
			if (!ExtractUdpBarbaPacket(&packet, orgPacketBuffer))
				return false;
			PacketHelper orgPacket(orgPacketBuffer);
			
			//prepare for NAT
			if (this->PeerFakeIp==0)
			{
				this->PeerFakeIp = BarbaUtils::ConvertStringIp("192.168.17.1");// + theApp->IpInc++;
				this->PeerLocalIp = orgPacket.GetSrcIp();
				memcpy(this->PeerEthAddress, packet.ethHeader->h_source, ETH_ALEN);
			}
			orgPacket.SetSrcIp(this->PeerFakeIp);
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
	else
	{
		if (send)
		{
			//static int i = 0;
			//printf("\norgPacket len: %d, sport%d, dport:%d", packet.GetPacketLen(), packet.GetSrcPort(), packet.GetDesPort());
			//if (packet.GetDesPort()!=1723)
				//return false;
			
			//Create Barba packet
			BYTE barbaPacket[MAX_ETHER_FRAME];
			size_t length = CreateUdpBarbaPacket(&packet, barbaPacket);
			
			//check result
			if (length==0)
				return false;

			//replace current packet with barba packet
			packet.SetEthPacket((ether_header_ptr)barbaPacket);
			CryptUdp(packet.ethHeader);
			packet.RecalculateChecksum();
			packetBuffer->m_Length = packet.GetPacketLen();

			return true;
		}
		else
		{
			//extract Barba packet
			BYTE orgPacketBuffer[MAX_ETHER_FRAME];
			CryptUdp(packet.ethHeader);
			if (!ExtractUdpBarbaPacket(&packet, orgPacketBuffer))
				return false;
			PacketHelper orgPacket(orgPacketBuffer);
	
			packet.SetEthPacket(orgPacket.ethHeader);
			packet.RecalculateChecksum();
			packetBuffer->m_Length = packet.GetPacketLen();

			static int i =0;
			//printf("\n%d, sum: %d", i++, packet.ipHeader->ip_sum);
			//printf("\norgPacket len: %d, sport%d, dport:%d\n", packet.GetPacketLen(), packet.GetSrcPort(), packet.GetDesPort());
			//BarbaUtils::PrintIp(orgPacket.GetSrcIp());
			//printf(" -- ");
			//BarbaUtils::PrintIp(orgPacket.GetDesIp());

			//extract org packet from barba packet
			return true;
		}

	}

	return false;
}
*/