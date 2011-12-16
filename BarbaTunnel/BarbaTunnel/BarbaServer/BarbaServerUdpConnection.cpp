#include "StdAfx.h"
#include "BarbaServerUdpConnection.h"


BarbaServerUdpConnection::BarbaServerUdpConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, 
	u_long clientIp, u_short clientPort, u_short tunnelPort, BYTE* clientRouteEthAddress)
	: BarbaServerConnection(configItem, clientVirtualIp, clientIp)
{
	this->ClientIp = clientIp;
	this->ClientPort = clientPort;
	this->TunnelPort = tunnelPort;
	memcpy_s(this->ClientRouteEthAddress, ETH_ALEN, clientRouteEthAddress, ETH_ALEN);
}

u_short BarbaServerUdpConnection::GetTunnelPort()
{
	return this->TunnelPort;
}

BarbaServerUdpConnection::~BarbaServerUdpConnection(void)
{
}

bool BarbaServerUdpConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer)
{
	CryptPacket(barbaPacket);
	PacketHelper orgPacket(orgPacketBuffer, 0);
	orgPacket.SetEthHeader(barbaPacket->ethHeader);
	return orgPacket.SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload()) && orgPacket.IsValidChecksum();
}

bool BarbaServerUdpConnection::CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer)
{
	packet->RecalculateChecksum();

	PacketHelper barbaPacket(barbaPacketBuffer, IPPROTO_UDP);
	barbaPacket.SetEthHeader(packet->ethHeader);
	barbaPacket.ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket.ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.ipHeader->ip_off = 0;
	barbaPacket.SetSrcIp(packet->GetSrcIp());
	barbaPacket.SetDesIp(this->ClientIp);
	barbaPacket.SetDesPort( this->ClientPort );
	barbaPacket.SetSrcPort(this->GetTunnelPort());
	barbaPacket.SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	barbaPacket.SetDesEthAddress(this->ClientRouteEthAddress);
	CryptPacket(&barbaPacket);
	return true;
}

//This function called when the connection should process the current IP
//In receive mode it should check signature after decrypt
//@return true if it process the packet
bool BarbaServerUdpConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		packet.SetDesIp(this->ClientLocalIp);

		//Create Barba packet
		BYTE barbaPacketBuffer[MAX_ETHER_FRAME];
		CreateUdpBarbaPacket(&packet, barbaPacketBuffer);
		PacketHelper barbaPacket(barbaPacketBuffer);

		packet.SetEthPacket(barbaPacket.ethHeader);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		this->SetWorkingState(packetBuffer->m_Length, send);
		return true;
	}
	else
	{
		//extract Barba packet
		BYTE orgPacketBuffer[MAX_ETHER_FRAME];
		if (!ExtractUdpBarbaPacket(&packet, orgPacketBuffer))
			return false;
		PacketHelper orgPacket(orgPacketBuffer);

		//Init First Attempt
		if (this->ClientLocalIp==0)
		{
			this->ClientLocalIp = orgPacket.GetSrcIp();
		}
			
		//prepare for NAT
		orgPacket.SetSrcIp(this->ClientVirtualIp);
		orgPacket.RecalculateChecksum();

		//replace current packet with barba packet
		packet.SetEthPacket(orgPacket.ethHeader);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		this->SetWorkingState(packetBuffer->m_Length, send);
		return true;
	}

}
