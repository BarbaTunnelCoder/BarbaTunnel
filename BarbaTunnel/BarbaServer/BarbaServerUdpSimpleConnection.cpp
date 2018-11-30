#include "StdAfx.h"
#include "BarbaServerUdpSimpleConnection.h"
#include "BarbaServerApp.h"


BarbaServerUdpSimpleConnection::BarbaServerUdpSimpleConnection(BarbaServerConfig* config, u_long clientVirtualIp, PacketHelper* initPacket)
	: BarbaServerConnection(config, clientVirtualIp, initPacket->GetSrcIp())
{
	ClientLocalIp = 0;
	LocalIp = initPacket->GetDesIp();
	ClientPort = initPacket->GetSrcPort();
	TunnelPort = initPacket->GetDesPort();
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
	orgPacket->SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload(), barbaPacket->GetUdpPayloadLen());
	return orgPacket->IsValidChecksum();
}

bool BarbaServerUdpSimpleConnection::CreateUdpBarbaPacket(PacketHelper* packet, PacketHelper* barbaPacket)
{
	barbaPacket->Reset(IPPROTO_UDP, sizeof iphdr + sizeof udphdr + packet->GetIpLen());
	barbaPacket->ipHeader->ip_ttl = 128;
	barbaPacket->ipHeader->ip_v = 4;
	barbaPacket->ipHeader->ip_id = 56;
	barbaPacket->ipHeader->ip_off = 0;
	barbaPacket->SetSrcIp(packet->GetSrcIp());
	barbaPacket->SetDesIp(ClientIp);
	barbaPacket->SetDesPort( ClientPort );
	barbaPacket->SetSrcPort( TunnelPort );
	barbaPacket->SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	EncryptPacket(barbaPacket);
	return true;
}

bool BarbaServerUdpSimpleConnection::ProcessOutboundPacket(PacketHelper* packet)
{
	if (!theApp->CheckMTUDecrement(packet->GetPacketLen(), sizeof iphdr + sizeof udphdr))
		return false;

	packet->SetDesIp(ClientLocalIp);
	packet->RecalculateChecksum();

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

	Log3(_T("Receiving packet with %d bytes."), orgPacket.GetPacketLen());
	SendPacketToInbound(&orgPacket);
	return true;

}


