#include "StdAfx.h"
#include "BarbaClientUdpConnection.h"


BarbaClientUdpConnection::BarbaClientUdpConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem, u_short clientPort, u_short tunnelPort)
	: BarbaClientConnection(config, configItem)
{
	this->ClientPort = clientPort;
	this->TunnelPort = tunnelPort;
}

u_short BarbaClientUdpConnection::GetTunnelPort()
{
	return this->TunnelPort;
}

BarbaClientUdpConnection::~BarbaClientUdpConnection(void)
{
}

bool BarbaClientUdpConnection::ShouldProcessPacket(PacketHelper* packet)
{
	//check outgoing packets
	if (packet->GetDesIp()==GetServerIp())
	{
		return ConfigItem->ShouldGrabPacket(packet);
	}
	//check incoming packets
	else if (packet->GetSrcIp()==GetServerIp())
	{
		return packet->ipHeader->ip_p==this->ConfigItem->GetTunnelProtocol() 
			&&	packet->GetSrcPort()==this->TunnelPort;
	}
	else
	{
		return false;
	}
}

bool BarbaClientUdpConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer)
{
	CryptPacket(barbaPacket);
	PacketHelper orgPacket(orgPacketBuffer, 0);
	orgPacket.SetEthHeader(barbaPacket->ethHeader);
	return orgPacket.SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload()) && orgPacket.IsValidChecksum();
}

bool BarbaClientUdpConnection::CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer)
{
	packet->RecalculateChecksum();

	PacketHelper barbaPacket(barbaPacketBuffer, IPPROTO_UDP);
	barbaPacket.SetEthHeader(packet->ethHeader);
	barbaPacket.ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket.ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.ipHeader->ip_off = packet->ipHeader->ip_off;
	barbaPacket.SetSrcIp(packet->GetSrcIp());
	barbaPacket.SetDesIp(this->GetServerIp());
	barbaPacket.SetSrcPort(this->ClientPort);
	barbaPacket.SetDesPort(this->TunnelPort);
	barbaPacket.SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	CryptPacket(&barbaPacket);
	return true;
}

bool BarbaClientUdpConnection::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		//Create Barba packet
		BYTE barbaPacket[MAX_ETHER_FRAME];
		if (!CreateUdpBarbaPacket(packet, barbaPacket))
			return false;
			
		//replace current packet with Barba packet
		packet->SetEthPacket((ether_header_ptr)barbaPacket);
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
		packet->SetEthPacket(orgPacket.ethHeader);
		this->SendPacketToMstcp(packet);
		return true;
	}
}