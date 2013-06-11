#include "StdAfx.h"
#include "BarbaClientUdpSimpleConnection.h"
#include "BarbaClientApp.h"


BarbaClientUdpSimpleConnection::BarbaClientUdpSimpleConnection(BarbaClientConfig* config, PacketHelper* initPacket)
	: BarbaClientConnection(config)
{
	ClientPort = initPacket->GetSrcPort();
	TunnelPort = config->TunnelPorts.GetRandomPort();
}

u_long BarbaClientUdpSimpleConnection::GetSessionId()
{
	return ClientPort;
}

BarbaClientUdpSimpleConnection::~BarbaClientUdpSimpleConnection(void)
{
}

bool BarbaClientUdpSimpleConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, PacketHelper* orgPacket)
{
	DecryptPacket(barbaPacket);
	orgPacket->SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload(), barbaPacket->GetUdpPayloadLen());
	return orgPacket->IsValidChecksum();
}

bool BarbaClientUdpSimpleConnection::CreateUdpBarbaPacket(PacketHelper* packet, PacketHelper*  barbaPacket)
{
	barbaPacket->Reset(IPPROTO_UDP, sizeof iphdr + sizeof udphdr + packet->GetIpLen());
	barbaPacket->ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket->ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket->ipHeader->ip_id = 56;
	barbaPacket->ipHeader->ip_off = packet->ipHeader->ip_off;
	barbaPacket->SetSrcIp(packet->GetSrcIp());
	barbaPacket->SetDesIp(GetConfig()->ServerIp);
	barbaPacket->SetSrcPort(ClientPort);
	barbaPacket->SetDesPort(TunnelPort);
	barbaPacket->SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	EncryptPacket(barbaPacket);
	return true;
}

bool BarbaClientUdpSimpleConnection::ProcessOutboundPacket(PacketHelper* packet)
{
	if (!theApp->CheckMTUDecrement(packet->GetPacketLen(), sizeof iphdr + sizeof udphdr + sizeof BarbaHeader))
		return false;

	//Create Barba packet
	PacketHelper barbaPacket;
	if (!CreateUdpBarbaPacket(packet, &barbaPacket))
		return false;

	Log3(_T("Sending packet with %d bytes."), packet->GetPacketLen());
	SendPacketToOutbound(&barbaPacket);
	return true;
}

bool BarbaClientUdpSimpleConnection::ProcessInboundPacket(PacketHelper* packet)
{
	if (packet->GetSrcPort()!=TunnelPort || packet->GetDesPort()!=ClientPort)
		return false;

	//extract Barba packet
	PacketHelper orgPacket;
	if (!ExtractUdpBarbaPacket(packet, &orgPacket))
	{
		BarbaLog(_T("Error: Detect packet with invalid checksum for UDP-Tunnel port %d! Check your key."), TunnelPort);
		return false;
	}

	Log3(_T("Receving packet with %d bytes."), orgPacket.GetPacketLen());
	SendPacketToInbound(&orgPacket);
	return true;
}