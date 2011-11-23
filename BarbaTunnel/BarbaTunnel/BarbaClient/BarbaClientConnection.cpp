#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaCrypt.h"

void BarbaClientConnection::CryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, Config->Key, Config->KeyCount);
}

void BarbaClientConnection::ReportConnection()
{
	TCHAR ip[100];
	PacketHelper::ConvertIpToString(Config->ServerIp, ip, _countof(ip));
	LPCTSTR mode = BarbaMode_ToString(ConfigItem->Mode);
	printf(_T("New connection! %s - %s:%d\n"), ip, mode, ConfigItem->TunnelPort);
}

bool BarbaClientConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer)
{
	CryptPacket(barbaPacket);
	PacketHelper orgPacket(orgPacketBuffer, 0);
	orgPacket.SetEthHeader(barbaPacket->ethHeader);
	return orgPacket.SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload()) && orgPacket.IsValidChecksum();
}

bool BarbaClientConnection::CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer)
{
	packet->RecalculateChecksum();

	PacketHelper barbaPacket(barbaPacketBuffer, IPPROTO_UDP);
	barbaPacket.SetEthHeader(packet->ethHeader);
	barbaPacket.ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket.ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.ipHeader->ip_off = 0;
	barbaPacket.SetSrcIp(packet->GetSrcIp());
	barbaPacket.SetDesIp(this->Config->ServerIp);
	barbaPacket.SetSrcPort(this->ClientPort);
	barbaPacket.SetDesPort(this->ConfigItem->TunnelPort);
	barbaPacket.SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	CryptPacket(&barbaPacket);
	return true;
}

//This function called when the connection should process the current IP
//In receive mode it should check signature after decrypt
//@return true if it process the packet
bool BarbaClientConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	this->LasNegotiationTime = GetTickCount();

	switch(ConfigItem->Mode){
	case BarbaModeTcpRedirect:
	case BarbaModeUdpRedirect:
		return ProcessPacketRedirect(packetBuffer);
	
	case BarbaModeUdpTunnel:
		return ProcessPacketUdpTunnel(packetBuffer);
	}

	return false;
}

bool BarbaClientConnection::ProcessPacketRedirect(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		packet.SetDesPort(this->ConfigItem->TunnelPort);
		CryptPacket(&packet);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
	}
	else
	{
		CryptPacket(&packet);
		if (!packet.IsValidChecksum())
			return false;
		packet.SetSrcPort(this->ConfigItem->RealPort);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
	}

	return true;
}

bool BarbaClientConnection::ProcessPacketUdpTunnel(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
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

		return true;
	}

}
