#include "StdAfx.h"
#include "PacketHelper.h"

//u_short ntohs( u_short netshort )
//{
//	PUCHAR	pBuffer;
//	u_short	nResult;
//
//	nResult = 0;
//	pBuffer = (PUCHAR )&netshort;
//
//	nResult = ( (pBuffer[ 0 ] << 8) & 0xFF00 )
//		| ( pBuffer[ 1 ] & 0x00FF );
//
//	return( nResult );
//}
//
//u_short htons( u_short netshort )
//{
//	return ntohs(netshort);
//}
//
//DWORD htonl( DWORD hostlong )
//{
//	ULONG nResult = hostlong >> 16;
//	USHORT upper = (USHORT) nResult & 0x0000FFFF;
//	USHORT lower = (USHORT) hostlong & 0x0000FFFF;
//
//	upper = htons( upper );
//	lower = htons( lower );
//
//    nResult = 0x10000 * lower + upper;
//	return( nResult );
//}
//
//DWORD ntohl( DWORD netshort )
//{
//	return htonl(netshort);
//}

PacketHelper::PacketHelper(void* packet, bool copy)
{
	memset(this->PacketBuffer, 0, MAX_ETHER_FRAME);
	if (copy)
	{
		memcpy_s(this->PacketBuffer, sizeof this->PacketBuffer, packet, MAX_ETHER_FRAME);
		packet = this->PacketBuffer;
	}
	ethHeader = (ether_header_ptr)packet;
	Reinit();
}

PacketHelper::PacketHelper()
{
	this->ethHeader = (ether_header_ptr)this->PacketBuffer;
	this->Reset(IPPROTO_IP);
}

PacketHelper::PacketHelper(u_char ipProtocol)
{
	this->ethHeader = (ether_header_ptr)this->PacketBuffer;
	this->Reset(ipProtocol);
}

void PacketHelper::Reset(u_char ipProtocol)
{
	memset(ethHeader, 0, MAX_ETHER_FRAME);
	ethHeader->h_proto = htons(ETH_P_IP);
	ipHeader = (iphdr*)(ethHeader + 1);
	ipHeader->ip_hl = 5;
	ipHeader->ip_p = ipProtocol;
	Reinit();

	if (ipProtocol==IPPROTO_TCP) SetTcpPayload(NULL, 0);
	else if (ipProtocol==IPPROTO_UDP) SetUdpPayload(NULL, 0);
}

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
	ipHeader->ip_len = htons( (u_short)(ipHeader->ip_hl*4 + tcpHeader->th_off*4 + len) );
}

void PacketHelper::SetUdpPayload(BYTE* payload, size_t len)
{
	BYTE* udpPayload = GetUdpPayload();
	int remain = MAX_ETHER_FRAME - (udpPayload-(BYTE*)ethHeader);
	memcpy_s(udpPayload, remain, payload, len);
	udpHeader->length = htons( (u_short)(sizeof(udphdr) + len) );
	ipHeader->ip_len = htons( (u_short)(ipHeader->ip_hl*4 + sizeof(udphdr) + len) );
}

void PacketHelper::RecalculateChecksum()
{
	RecalculateIPChecksum(ipHeader, true);
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
	if ( ethHeader->h_proto==ntohs(ETH_P_IP))
		len = sizeof (ether_header) + ntohs(((iphdr*)(ethHeader + 1))->ip_len);
	memcpy_s(this->ethHeader, MAX_ETHER_FRAME, ethHeader, len);
	Reinit();
}

bool PacketHelper::SetIpPacket(iphdr_ptr ipHeader)
{
	if (ntohs(ipHeader->ip_len)>MAX_ETHER_FRAME-sizeof ether_header)
		return false; //big IP!

	this->ethHeader->h_proto = htons(ETH_P_IP);
	this->ipHeader = (iphdr*)(ethHeader + 1);
	memcpy_s(this->ipHeader, MAX_ETHER_FRAME-sizeof ether_header, ipHeader, ntohs(ipHeader->ip_len));
	Reinit();
	return true;
}

void PacketHelper::SetSrcEthAddress(BYTE* address)
{
	memcpy_s(this->ethHeader->h_source, ETH_ALEN, address, ETH_ALEN);
}

void PacketHelper::SetDesEthAddress(BYTE* address)
{
	memcpy_s(this->ethHeader->h_dest, ETH_ALEN, address, ETH_ALEN);
}

BYTE* PacketHelper::GetIpExtraHeader()
{
	return (BYTE*)(ipHeader + 1);
}

size_t PacketHelper::GetIpExtraHeaderLen()
{
	return ipHeader->ip_hl*4 - sizeof (iphdr);
}

BYTE* PacketHelper::GetTcpExtraHeader()
{
	return (BYTE*)(tcpHeader + 1);
}

size_t PacketHelper::GetTcpExtraHeaderLen()
{
	return tcpHeader->th_off*4 - sizeof (tcphdr);
}

bool PacketHelper::IsValidChecksum()
{
	return IsIp() && PacketHelper::IsValidIPChecksum(ipHeader);
}

bool PacketHelper::IsValidIPChecksum(iphdr_ptr ipHeader)
{
	if (ntohs(ipHeader->ip_len)>MAX_ETHER_FRAME-sizeof ether_header)
		return false; //big IP!

	BYTE buffer[MAX_ETHER_FRAME];
	iphdr_ptr ipPacket = (iphdr_ptr)buffer;
	memcpy(ipPacket, ipHeader, ntohs(ipHeader->ip_len));
	PacketHelper::RecalculateIPChecksum((iphdr_ptr)ipPacket, false);
	return ipPacket->ip_sum==ipPacket->ip_sum;
}


void PacketHelper::RecalculateIPChecksum(iphdr_ptr pIpHeader, bool calculateProtoCheckSum)
{
	if (ntohs(pIpHeader->ip_len)>MAX_ETHER_FRAME-sizeof ether_header)
		return ; //big IP!


	if (calculateProtoCheckSum)
	{
		if (pIpHeader->ip_p==IPPROTO_TCP) RecalculateTCPChecksum(pIpHeader);
		else if (pIpHeader->ip_p==IPPROTO_UDP) RecalculateUDPChecksum(pIpHeader);
		else if (pIpHeader->ip_p==IPPROTO_ICMP) RecalculateICMPChecksum(pIpHeader);
		//else _tprintf_s("Unknown protocol for checksum!\n"); 
	}


	u_short word16;
	unsigned int sum = 0;
	unsigned int i = 0;
	PUCHAR buff;

	// Initialize checksum to zero
	pIpHeader->ip_sum = 0;
	buff = (PUCHAR)pIpHeader;

	// Calculate IP header checksum
	for (i = 0; i < pIpHeader->ip_hl*sizeof(DWORD); i=i+2)
	{
		word16 = ((buff[i]<<8)&0xFF00)+(buff[i+1]&0xFF);
		sum = sum+word16; 
	}

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
	while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);

	// Take the one's complement of sum
	sum = ~sum;

	pIpHeader->ip_sum = htons((unsigned short) sum);
}


u_short PacketHelper::CheckSum(u_short *buffer, int size)
{
	unsigned long cksum=0;
	while(size >1)
	{
		cksum+=*buffer++;
		size -=sizeof(u_short);
	}
	if(size)
		cksum += *(UCHAR*)buffer;

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >>16);
	return (u_short)(~cksum);
}

void PacketHelper::RecalculateTCPChecksum(iphdr_ptr pIpHeader)
{
	tcphdr_ptr pTcpHeader = NULL;
	unsigned short word16, padd = 0;
	unsigned int i, sum = 0;
	PUCHAR buff;
	DWORD dwTcpLen;

	// Sanity check
	if (pIpHeader->ip_p == IPPROTO_TCP)
	{
		pTcpHeader = (tcphdr_ptr)(((PUCHAR)pIpHeader) + sizeof(DWORD)*pIpHeader->ip_hl);
	}
	else
		return;
	
	dwTcpLen = ntohs(pIpHeader->ip_len) - pIpHeader->ip_hl*4;//pPacket->m_Length - ((PUCHAR)(pTcpHeader) - pPacket->m_IBuffer);

	if ( (dwTcpLen/2)*2 != dwTcpLen )
	{
		padd=1;
		((BYTE*)pIpHeader)[pIpHeader->ip_hl*4+dwTcpLen] = 0;
	}

	buff = (PUCHAR)pTcpHeader;
	pTcpHeader->th_sum = 0;

	// make 16 bit words out of every two adjacent 8 bit words and 
	// calculate the sum of all 16 vit words
	for (i=0; i< dwTcpLen+padd; i=i+2){
		word16 =((buff[i]<<8)&0xFF00)+(buff[i+1]&0xFF);
		sum = sum + (unsigned long)word16;
	}
	
	// add the TCP pseudo header which contains:
	// the IP source and destination addresses,

	sum = sum + ntohs(pIpHeader->ip_src.S_un.S_un_w.s_w1) + ntohs(pIpHeader->ip_src.S_un.S_un_w.s_w2);
	sum = sum + ntohs(pIpHeader->ip_dst.S_un.S_un_w.s_w1) + ntohs(pIpHeader->ip_dst.S_un.S_un_w.s_w2);
	
	// the protocol number and the length of the TCP packet
	sum = sum + IPPROTO_TCP + (unsigned short)dwTcpLen;

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum;

	pTcpHeader->th_sum = htons((unsigned short)sum);
}

void PacketHelper::RecalculateICMPChecksum(iphdr_ptr pIpHeader)
{
	unsigned short word16, padd = 0;
	unsigned int i, sum = 0;
	PUCHAR buff;
	DWORD dwIcmpLen;
	icmphdr_ptr pIcmpHeader = NULL;

	// Sanity check
	if (pIpHeader->ip_p == IPPROTO_ICMP)
	{
		pIcmpHeader = (icmphdr_ptr)(((PUCHAR)pIpHeader) + sizeof(DWORD)*pIpHeader->ip_hl);
	}
	else
		return;

	dwIcmpLen = ntohs(pIpHeader->ip_len) - pIpHeader->ip_hl*4;

	if ( (dwIcmpLen/2)*2 != dwIcmpLen )
	{
		padd=1;
		((BYTE*)pIpHeader)[pIpHeader->ip_hl*4+dwIcmpLen] = 0;
	}

	buff = (PUCHAR)pIcmpHeader;
	pIcmpHeader->checksum = 0;

	// make 16 bit words out of every two adjacent 8 bit words and 
	// calculate the sum of all 16 bit words
	for (i=0; i< dwIcmpLen+padd; i=i+2){
		word16 =((buff[i]<<8)&0xFF00)+(buff[i+1]&0xFF);
		sum = sum + (unsigned long)word16;
	}
	
	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum;

	pIcmpHeader->checksum = ntohs((unsigned short)sum);
}

void PacketHelper::RecalculateUDPChecksum(iphdr_ptr pIpHeader)
{
	udphdr_ptr pUdpHeader = NULL;
	unsigned short word16, padd = 0;
	unsigned int i, sum = 0;
	PUCHAR buff;
	DWORD dwUdpLen;

	// Sanity check
	if (pIpHeader->ip_p == IPPROTO_UDP)
	{
		pUdpHeader = (udphdr_ptr)(((PUCHAR)pIpHeader) + sizeof(DWORD)*pIpHeader->ip_hl);
	}
	else
		return;
	
	dwUdpLen = ntohs(pIpHeader->ip_len) - pIpHeader->ip_hl*4;//pPacket->m_Length - ((PUCHAR)(pTcpHeader) - pPacket->m_IBuffer);

	if ( (dwUdpLen/2)*2 != dwUdpLen )
	{
		padd=1;
		((BYTE*)pIpHeader)[pIpHeader->ip_hl*4+dwUdpLen] = 0;
	}

	buff = (PUCHAR)pUdpHeader;
	pUdpHeader->th_sum = 0;

	// make 16 bit words out of every two adjacent 8 bit words and 
	// calculate the sum of all 16 vit words
	for (i=0; i< dwUdpLen+padd; i=i+2){
		word16 =((buff[i]<<8)&0xFF00)+(buff[i+1]&0xFF);
		sum = sum + (unsigned long)word16;
	}
	
	// add the UDP pseudo header which contains:
	// the IP source and destination addresses,

	sum = sum + ntohs(pIpHeader->ip_src.S_un.S_un_w.s_w1) + ntohs(pIpHeader->ip_src.S_un.S_un_w.s_w2);
	sum = sum + ntohs(pIpHeader->ip_dst.S_un.S_un_w.s_w1) + ntohs(pIpHeader->ip_dst.S_un.S_un_w.s_w2);
	
	// the protocol number and the length of the UDP packet
	sum = sum + IPPROTO_UDP + (unsigned short)dwUdpLen;

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum;

	pUdpHeader->th_sum = ntohs((unsigned short)sum);
}

DWORD PacketHelper::ConvertStringIp(LPCTSTR pszIp)
{
	TCHAR ip[100];
	_tcscpy_s(ip, pszIp);

	BYTE ret[4] = {0};
	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(ip, _T("."), &currentPos);
		
	int index = 0;
	while (token!=NULL && index<4)
	{
		ret[index] = (BYTE)_tcstoul(token, NULL, 0);
		token = _tcstok_s(NULL, _T("."), &currentPos);
		index++;
	}
		
	return *(DWORD*)ret;
}

void PacketHelper::ConvertIpToString(DWORD ip, TCHAR* buffer, rsize_t bufferCount)
{
	_stprintf_s(buffer, bufferCount, _T("%d.%d.%d.%d"), LOBYTE(LOWORD(ip)), HIBYTE(LOWORD(ip)), LOBYTE(HIWORD(ip)), HIBYTE(HIWORD(ip)));
}

//@param value eg: TCP:80; TCP:*; *
BYTE PacketHelper::ConvertStringProtocol(LPCTSTR protocol)
{
	if (_tcsicmp(protocol, _T("ICMP"))==0) return IPPROTO_ICMP;
	else if (_tcsicmp(protocol, _T("IGMP"))==0) return IPPROTO_IGMP;
	else if (_tcsicmp(protocol, _T("GGP"))==0) return IPPROTO_GGP;
	else if (_tcsicmp(protocol, _T("TCP"))==0) return IPPROTO_TCP;
	else if (_tcsicmp(protocol, _T("PUP"))==0) return IPPROTO_PUP;
	else if (_tcsicmp(protocol, _T("UDP"))==0) return IPPROTO_UDP;
	else if (_tcsicmp(protocol, _T("IDP"))==0) return IPPROTO_IDP;
	else if (_tcsicmp(protocol, _T("GRE"))==0) return IPPROTO_GRE;
	else if (_tcsicmp(protocol, _T("ND"))==0) return IPPROTO_ND;

	return (BYTE)_tcstoul(protocol, NULL, 0);
}

//@param value eg: TCP:80; TCP:*; *
LPCTSTR PacketHelper::ConvertProtocolToString(BYTE protocol)
{
	switch(protocol){
	case IPPROTO_ICMP: return _T("ICMP");
	case IPPROTO_IGMP: return _T("IGMP");
	case IPPROTO_GGP: return _T("GGP");
	case IPPROTO_TCP: return _T("TCP");
	case IPPROTO_PUP: return _T("PUP");
	case IPPROTO_UDP: return _T("UDP");
	case IPPROTO_IDP: return _T("IDP");
	case IPPROTO_GRE: return _T("GRE");
	case IPPROTO_ND: return _T("ND");
	default: return _T("?");
	}
}
