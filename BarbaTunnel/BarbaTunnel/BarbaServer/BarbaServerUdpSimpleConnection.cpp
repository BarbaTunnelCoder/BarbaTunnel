#include "StdAfx.h"
#include "BarbaServerUdpSimpleConnection.h"
#include "BarbaServerApp.h"


BarbaServerUdpSimpleConnection::BarbaServerUdpSimpleConnection(BarbaServerConfig* config, u_long clientVirtualIp, PacketHelper* initPacket)
	: BarbaServerConnection(config, clientVirtualIp, initPacket->GetSrcIp())
{
	ClientLocalIp = 0;
	ClientPort = initPacket->GetSrcPort();
	TunnelPort = initPacket->GetDesPort();
	memcpy_s(ClientRouteEthAddress, ETH_ALEN, initPacket->ethHeader->h_source, ETH_ALEN);
}

u_long BarbaServerUdpSimpleConnection::GetSessionId()
{
	return ClientPort;
}

BarbaServerUdpSimpleConnection::~BarbaServerUdpSimpleConnection(void)
{
}

bool BarbaServerUdpSimpleConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, PacketHelper* orgPacket)
{
	DecryptPacket(barbaPacket);
	orgPacket->SetEthHeader(barbaPacket->ethHeader);
	orgPacket->SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload(), barbaPacket->GetUdpPayloadLen());
	return orgPacket->IsValidChecksum();
}

bool BarbaServerUdpSimpleConnection::CreateUdpBarbaPacket(PacketHelper* packet, PacketHelper* barbaPacket)
{
	packet->RecalculateChecksum();

	barbaPacket->Reset(IPPROTO_UDP, sizeof iphdr + sizeof udphdr + packet->GetIpLen());
	barbaPacket->SetEthHeader(packet->ethHeader);
	barbaPacket->ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket->ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket->ipHeader->ip_id = 56;
	barbaPacket->ipHeader->ip_off = 0;
	barbaPacket->SetSrcIp(packet->GetSrcIp());
	barbaPacket->SetDesIp(ClientIp);
	barbaPacket->SetDesPort( ClientPort );
	barbaPacket->SetSrcPort( TunnelPort );
	barbaPacket->SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	barbaPacket->SetDesEthAddress(ClientRouteEthAddress);
	EncryptPacket(barbaPacket);
	return true;
}

bool BarbaServerUdpSimpleConnection::ProcessOutboundPacket(PacketHelper* packet)
{
	if (!theApp->CheckMTUDecrement(packet->GetPacketLen(), sizeof iphdr + sizeof udphdr + sizeof BarbaHeader))
		return false;

	packet->SetDesIp(ClientLocalIp);

	//Create Barba packet
	PacketHelper barbaPacket;
	CreateUdpBarbaPacket(packet, &barbaPacket);

	Log3(_T("Sending packet with %d bytes."), packet->GetPacketLen());
	SendPacketToOutbound(&barbaPacket);
	return true;
}

bool BarbaServerUdpSimpleConnection::ProcessInboundPacket(PacketHelper* packet)
{
	if (packet->GetSrcPort()!=ClientPort)
		return false;

	//extract Barba packet
	PacketHelper orgPacket;
	if (!ExtractUdpBarbaPacket(packet, &orgPacket))
	{
		BarbaLog(_T("Error: Detect packet with invalid checksum for UDP-Tunnel port %d! Check your key."), TunnelPort);
		return false;
	}

	//Initialize First Attempt
	ClientLocalIp = orgPacket.GetSrcIp();

	//prepare for NAT
	orgPacket.SetSrcIp(ClientVirtualIp);

	Log3(_T("Receving packet with %d bytes."), orgPacket.GetPacketLen());
	SendPacketToInbound(&orgPacket);
	return true;

}


