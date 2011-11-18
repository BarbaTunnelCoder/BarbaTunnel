#include "stdafx.h"

/*
void printPacketInfo(ether_header_ptr ethHeader)
{
	PacketHelper packet(ethHeader);

	if (ntohs(ethHeader->h_proto)!=ETH_P_IP)
	{
		printf("Packet is not IP paket!\n");
		return;
	}

	//printf("", packet.ipHeader->ip_sum);

	//int ipLen = ntohs( ipHeader->ip_len );
	//int tcpPayloadLen = ipLen - ipHeader->ip_hl*4 - tcpHeader->th_off*4;
	//printf("sport: %d, dport:%d, tcpFlag:%x, ipLen:%d, ipHL: %d, tcpOff: %d, tcpPayloadLen:%d, ipCheckSum:%d\n", ntohs(tcpHeader->th_sport), ntohs(tcpHeader->th_dport), tcpHeader->th_off, 
		//ipLen, ipHeader->ip_hl, tcpHeader->th_off, tcpPayloadLen, BarbaUtils::CheckSum((USHORT*)ipHeader, ipLen));

}

void SendPing(INTERMEDIATE_BUFFER* packet)
{
	printf("Sending Ping..\n");

	ether_header_ptr ethHeader = (ether_header*)packet->m_IBuffer;
	iphdr_ptr ipHeader = (iphdr*)(ethHeader + 1);
	int orgIpLen = ntohs(ipHeader->ip_len);

	//my ping
	BYTE buffer[MAX_ETHER_FRAME] = {0};
	memset(buffer, 7, MAX_ETHER_FRAME);
	ether_header_ptr eth = (ether_header_ptr)buffer;
	iphdr_ptr ip = (iphdr_ptr)(buffer + sizeof ether_header);

	memcpy(eth->h_source, ethHeader->h_source, ETH_ALEN);
	memcpy(eth->h_dest, ethHeader->h_dest, ETH_ALEN);
	eth->h_proto = ethHeader->h_proto;
	//memcpy(eth, ethHeader, sizeof ether_header);


	ip->ip_src.S_un.S_addr = BarbaUtils::ConvertStringIp(_T("192.168.0.21"));
	ip->ip_dst.S_un.S_addr = BarbaUtils::ConvertStringIp(_T("76.72.163.224"));
	ip->ip_p = IPPROTO_ICMP;
	ip->ip_v = 4;
	ip->ip_hl = 5;
	ip->ip_id = 0;
	ip->ip_tos = 0;
	ip->ip_off = 0;
	ip->ip_ttl = 128;
	ip->ip_len = htons( ntohs(15360) + 0);
	int dataLen = orgIpLen-ipHeader->ip_hl*4;
	memcpy((BYTE*)ip + ip->ip_hl*4, (BYTE*)ipHeader + ipHeader->ip_hl*4, orgIpLen-ipHeader->ip_hl*4);

	BarbaUtils::RecalculateIPChecksum(ip);

	int len = sizeof ether_header + ntohs(ip->ip_len);
	memcpy(packet->m_IBuffer, eth, MAX_ETHER_FRAME);
	packet->m_Length = len;
}
*/