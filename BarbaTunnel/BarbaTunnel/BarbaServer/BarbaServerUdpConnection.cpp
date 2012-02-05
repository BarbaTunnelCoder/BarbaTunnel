#include "StdAfx.h"
#include "BarbaServerUdpConnection.h"
#include "BarbaServerApp.h"


BarbaServerUdpConnection::BarbaServerUdpConnection(BarbaServerConfig* config, u_long clientVirtualIp, 
	u_long clientIp, u_short clientPort, u_short tunnelPort, BYTE* clientRouteEthAddress)
	: BarbaServerConnection(config, clientVirtualIp, clientIp)
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

bool BarbaServerUdpConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, PacketHelper* orgPacket)
{
	DecryptPacket(barbaPacket);
	orgPacket->SetEthHeader(barbaPacket->ethHeader);
	orgPacket->SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload());
	return orgPacket->IsValidChecksum();
}

bool BarbaServerUdpConnection::CreateUdpBarbaPacket(PacketHelper* packet, PacketHelper* barbaPacket)
{
	packet->RecalculateChecksum();

	barbaPacket->Reset(IPPROTO_UDP, sizeof iphdr + sizeof udphdr + packet->GetIpLen());
	barbaPacket->SetEthHeader(packet->ethHeader);
	barbaPacket->ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket->ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket->ipHeader->ip_id = 56;
	barbaPacket->ipHeader->ip_off = 0;
	barbaPacket->SetSrcIp(packet->GetSrcIp());
	barbaPacket->SetDesIp(this->ClientIp);
	barbaPacket->SetDesPort( this->ClientPort );
	barbaPacket->SetSrcPort(this->GetTunnelPort());
	barbaPacket->SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	barbaPacket->SetDesEthAddress(this->ClientRouteEthAddress);
	EncryptPacket(barbaPacket);
	return true;
}

//This function called when the connection should process the current IP
//In receive mode it should check signature after decrypt
//@return true if it process the packet
bool BarbaServerUdpConnection::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		if (!theApp->CheckMTUDecrement(packet->GetPacketLen(), sizeof iphdr + sizeof tcphdr + sizeof BarbaHeader))
			return false;

		packet->SetDesIp(this->ClientLocalIp);

		//Create Barba packet
		PacketHelper barbaPacket;
		CreateUdpBarbaPacket(packet, &barbaPacket);
		SendPacketToOutbound(&barbaPacket);
		return true;
	}
	else
	{
		//extract Barba packet
		PacketHelper orgPacket;
		if (!ExtractUdpBarbaPacket(packet, &orgPacket))
		{
			BarbaLog(_T("Error: Detect packet with invalid checksum for UDP-Tunnel port %d! Check your key."), this->GetTunnelPort());
			return false;
		}
		
		//Initialize First Attempt
		if (this->ClientLocalIp==0)
			this->ClientLocalIp = orgPacket.GetSrcIp();
			
		//prepare for NAT
		orgPacket.SetSrcIp(this->ClientVirtualIp);
		SendPacketToInbound(&orgPacket);
		return true;
	}
}
