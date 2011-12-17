#include "StdAfx.h"
#include "BarbaServerUdpConnection.h"


BarbaServerUdpConnection::BarbaServerUdpConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, 
	u_long clientIp, u_short clientPort, u_short tunnelPort, BYTE* clientRouteEthAddress)
	: BarbaServerConnection(configItem, clientVirtualIp, clientIp)
{
	this->ClientIp = clientIp;
	this->ClientPort = clientPort;
	this->ClientLocalIp = 0;
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

bool BarbaServerUdpConnection::ShouldProcessPacket(PacketHelper* packet)
{
	return 
		(packet->GetDesIp()==this->GetClientVirtualIp()) || //check outgoing packets
		(packet->GetSrcIp()==this->ClientIp && packet->ipHeader->ip_p==BarbaMode_GetProtocol(GetMode()) && packet->GetSrcPort()==this->ClientPort && packet->GetDesPort()==this->GetTunnelPort() );  //check incoming packets
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
bool BarbaServerUdpConnection::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		packet->SetDesIp(this->ClientLocalIp);

		//Create Barba packet
		BYTE barbaPacketBuffer[MAX_ETHER_FRAME];
		CreateUdpBarbaPacket(packet, barbaPacketBuffer);
		PacketHelper barbaPacket(barbaPacketBuffer);

		packet->SetEthPacket(barbaPacket.ethHeader);
		this->SendPacketToAdapter(packet);
		return true;
	}
	else
	{
		//extract Barba packet
		BYTE orgPacketBuffer[MAX_ETHER_FRAME];
		if (!ExtractUdpBarbaPacket(packet, orgPacketBuffer))
			return false;
		PacketHelper orgPacket(orgPacketBuffer);

		//Initialize First Attempt
		if (this->ClientLocalIp==0)
			this->ClientLocalIp = orgPacket.GetSrcIp();
			
		//prepare for NAT
		orgPacket.SetSrcIp(this->ClientVirtualIp);
		orgPacket.RecalculateChecksum();

		//replace current packet with Barba packet
		packet->SetEthPacket(orgPacket.ethHeader);
		this->SendPacketToMstcp(packet);
		return true;
	}

}
