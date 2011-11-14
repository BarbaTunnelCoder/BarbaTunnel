#include "StdAfx.h"
#include "PacketHelper.h"
#include "BarbaUtils.h"

void PacketHelper::Reinit()
{
	ipHeader = NULL;
	tcpHeader = NULL;
	udpHeader = NULL;

	if (ntohs(ethHeader->h_proto)==ETH_P_IP)
		ipHeader = (iphdr*)(ethHeader + 1);
			
	if (ipHeader!=NULL && ipHeader->ip_p==IPPROTO_TCP)
		tcpHeader = (tcphdr_ptr)(((BYTE*)ipHeader) + ipHeader->ip_hl*4);

	if (ipHeader!=NULL && ipHeader->ip_p==IPPROTO_UDP)
		udpHeader = (udphdr_ptr)(((BYTE*)ipHeader) + ipHeader->ip_hl*4);
}

PacketHelper::PacketHelper(void* packet)
{
	ethHeader = (ether_header_ptr)packet;
	Reinit();
}

PacketHelper::PacketHelper(void* packet, u_char ipProtocol)
{
	memset(packet, 0, MAX_ETHER_FRAME);
	ethHeader = (ether_header_ptr)packet;
	ethHeader->h_proto = htons(ETH_P_IP);

	ipHeader = (iphdr*)(ethHeader + 1);
	ipHeader->ip_hl = 5;
	ipHeader->ip_p = ipProtocol;
	Reinit();

	if (ipProtocol==IPPROTO_TCP) SetTcpPayload(NULL, 0);
	else if (ipProtocol==IPPROTO_UDP) SetUdpPayload(NULL, 0);
}

void PacketHelper::SetDesPort(u_short port)
{
	if (IsTcp())
		tcpHeader->th_dport = htons(port);
	else
		udpHeader->th_dport = htons(port);
}

void PacketHelper::SetSrcPort(u_short port)
{
	if (IsTcp())
		tcpHeader->th_sport = htons(port);
	else
		udpHeader->th_sport = htons(port);
}

void PacketHelper::SetTcpPayload(BYTE* payload, size_t len)
{
	BYTE* tcpPayload = GetTcpPayload();
	int remain = MAX_ETHER_FRAME - (tcpPayload-(BYTE*)ethHeader);
	memcpy_s(tcpPayload, remain, payload, len);
	ipHeader->ip_len = htons( ipHeader->ip_hl*4 + tcpHeader->th_off*4 + len );
}

void PacketHelper::SetUdpPayload(BYTE* payload, size_t len)
{
	BYTE* udpPayload = GetUdpPayload();
	int remain = MAX_ETHER_FRAME - (udpPayload-(BYTE*)ethHeader);
	memcpy_s(udpPayload, remain, payload, len);
	udpHeader->length = htons(sizeof(udphdr) + len);
	ipHeader->ip_len = htons( ipHeader->ip_hl*4 + sizeof(udphdr) + len );
}

void PacketHelper::RecalculateChecksum()
{
	BarbaUtils::RecalculateIPChecksum(ipHeader, true);
}

u_short PacketHelper::GetDesPort()
{
	if (IsTcp()) return ntohs(tcpHeader->th_dport);
	if (IsUdp()) return ntohs(udpHeader->th_dport);
	return 0;
}

u_short PacketHelper::GetSrcPort()
{
	if (IsTcp()) return ntohs(tcpHeader->th_sport);
	if (IsUdp()) return ntohs(udpHeader->th_sport);
	return 0;
}

void PacketHelper::SetEthHeader(ether_header_ptr ethHeader)
{
	memcpy_s(this->ethHeader, sizeof ether_header, ethHeader, sizeof ether_header);
	Reinit();
}

void PacketHelper::SetEthPacket(ether_header_ptr ethHeader)
{
	size_t len = MAX_ETHER_FRAME;
	if ( ethHeader->h_proto=ntohs(ETH_P_IP))
		len = sizeof (ether_header) + ntohs(((iphdr*)(ethHeader + 1))->ip_len);
	memcpy_s(this->ethHeader, MAX_ETHER_FRAME, ethHeader, len);
	Reinit();
}

void PacketHelper::SetIpPacket(iphdr_ptr ipHeader)
{
	this->ethHeader->h_proto = htons(ETH_P_IP);
	this->ipHeader = (iphdr*)(ethHeader + 1);
	memcpy_s(this->ipHeader, MAX_ETHER_FRAME-sizeof ether_header, ipHeader, ntohs(ipHeader->ip_len));
	Reinit();
}

void PacketHelper::SetSrcEthAddress(BYTE* address)
{
	memcpy_s(this->ethHeader->h_source, ETH_ALEN, address, ETH_ALEN);
}

void PacketHelper::SetDesEthAddress(BYTE* address)
{
	memcpy_s(this->ethHeader->h_dest, ETH_ALEN, address, ETH_ALEN);
}
