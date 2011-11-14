#include "stdafx.h"
#include "BarbaUtils.h"

USHORT ntohs( USHORT netshort )
{
	PUCHAR	pBuffer;
	USHORT	nResult;

	nResult = 0;
	pBuffer = (PUCHAR )&netshort;

	nResult = ( (pBuffer[ 0 ] << 8) & 0xFF00 )
		| ( pBuffer[ 1 ] & 0x00FF );

	return( nResult );
}

USHORT htons( USHORT netshort )
{
	return ntohs(netshort);
}

void BarbaUtils::GetModuleFolder(TCHAR* folder)
{
	::GetModuleFileName(NULL, folder, MAX_PATH);
	for (int i=_tcsclen(folder); i>=0; i--)
	{
		if (folder[i]=='\\' || folder[i]=='/')
		{
			folder[i] = 0;
			break;
		}
		folder[i] = 0;
	}
}

void BarbaUtils::PrintIp(DWORD ip)
{
	printf("%d.%d.%d.%d", LOBYTE(LOWORD(ip)), HIBYTE(LOWORD(ip)), LOBYTE(HIWORD(ip)), HIBYTE(HIWORD(ip)));
}

DWORD BarbaUtils::ConvertStringIp(LPCTSTR pszIp)
{
	TCHAR ip[100];
	_tcscpy_s(ip, pszIp);

	BYTE ret[4] = {0};
	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(ip, _T("."), &currentPos);
		
	int index = 0;
	while (token!=NULL && index<4)
	{
		TCHAR* end = NULL;
		ret[index] = (BYTE)_tcstoul(token, &end, 0);
		token = _tcstok_s(NULL, _T("."), &currentPos);
		index++;
	}
		
	return *(DWORD*)ret;
}

bool BarbaUtils::GetProtocolAndPort(LPCTSTR value, BYTE* protocol, int* port)
{
	bool ret = false;
	*protocol = 0;
	*port = 0;
	TCHAR buffer[100];
	_tcscpy_s(buffer, value);

	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(buffer, _T(":"), &currentPos);
		
	int index = 0;
	while (token!=NULL && index<2)
	{
		TCHAR* end = NULL;
		if (index==0 && _tcsicmp(token, _T("*"))==0) {*protocol = 0; ret = true;}
		else if (index==0) {*protocol = ConvertStringpProtocol(token); ret = *protocol!=0;}
		if (index==1) *port = (int)_tcstoul(token, &end, 0);
		token = _tcstok_s(NULL, _T("."), &currentPos);
		index++;
	}

	return ret;
}

//@param value eg: TCP:80; TCP:*; *
BYTE BarbaUtils::ConvertStringpProtocol(LPCTSTR protocol)
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

	TCHAR* end = NULL;
	return (BYTE)_tcstoul(protocol, &end, 0);
}

void BarbaUtils::RecalculateTCPChecksum2(iphdr_ptr ipHeader)
{
	tcphdr_ptr tcpHeader = (tcphdr_ptr)(((PUCHAR)ipHeader) + sizeof(DWORD)*ipHeader->ip_hl);
	tcpHeader->th_sum = 0;

	// pseudo header
	struct pseudohdr
	{
		unsigned int sourceIP;
		unsigned int destIP;
		unsigned char placholder;
		unsigned char proto;
		unsigned short tcp_len;
	}pshdr;

	pshdr.sourceIP = ipHeader->ip_src.S_un.S_addr;
	pshdr.destIP = ipHeader->ip_dst.S_un.S_addr;
	pshdr.placholder = 0;
	pshdr.proto = ipHeader->ip_p;
	pshdr.tcp_len = htons( ntohs(ipHeader->ip_len) - ipHeader->ip_hl*4);

	void* buffer;
	buffer = malloc( sizeof(pseudohdr) + ntohs(pshdr.tcp_len) ); 
	memcpy(buffer, &pshdr, sizeof(pseudohdr));
	memcpy((BYTE*)buffer + sizeof(pseudohdr), tcpHeader, ntohs(pshdr.tcp_len));
	tcpHeader->th_sum = CheckSum( (u_short*)buffer, sizeof(pseudohdr) + ntohs(pshdr.tcp_len));
	free(buffer);
}

void BarbaUtils::RecalculateIPChecksum(iphdr_ptr pIpHeader, bool calculateProtoCheckSum)
{
	if (calculateProtoCheckSum)
	{
		if (pIpHeader->ip_p==IPPROTO_TCP) RecalculateTCPChecksum(pIpHeader);
		else if (pIpHeader->ip_p==IPPROTO_UDP) RecalculateUDPChecksum(pIpHeader);
		else if (pIpHeader->ip_p==IPPROTO_ICMP) RecalculateICMPChecksum(pIpHeader);
		//else printf("Unknown protocol for checksum!\n"); 
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


///////////////////
// checksum function:
u_short BarbaUtils::CheckSum(USHORT *buffer, int size)
{
	unsigned long cksum=0;
	while(size >1)
	{
		cksum+=*buffer++;
		size -=sizeof(USHORT);
	}
	if(size)
		cksum += *(UCHAR*)buffer;

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >>16);
	return (u_short)(~cksum);
}

void BarbaUtils::RecalculateTCPChecksum(iphdr_ptr pIpHeader)
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

void BarbaUtils::RecalculateICMPChecksum(iphdr_ptr pIpHeader)
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

void BarbaUtils::RecalculateUDPChecksum(iphdr_ptr pIpHeader)
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

int BarbaUtils::CreateUdpPacket(BYTE* srcEthAddress, BYTE* desEthAddress, DWORD srcIp, DWORD desIP, u_short srcPort, u_short desPort, BYTE* buffer, size_t bufferCount, ether_header_ptr udpPacket)
{
	//Ethernet
	memcpy_s(udpPacket->h_dest, ETH_ALEN, desEthAddress, ETH_ALEN);
	memcpy_s(udpPacket->h_source, ETH_ALEN, srcEthAddress, ETH_ALEN);
	udpPacket->h_proto = htons(ETH_P_IP);

	//ip
	iphdr_ptr ipHeader = (iphdr*)(udpPacket + 1);
	memset(ipHeader, 0, sizeof iphdr);
	ipHeader->ip_src.S_un.S_addr = srcIp;
	ipHeader->ip_dst.S_un.S_addr = desIP;
	ipHeader->ip_hl = 5;
	ipHeader->ip_id = 56;
	ipHeader->ip_ttl = 128;
	ipHeader->ip_off = 0;
	ipHeader->ip_len = htons( sizeof iphdr + sizeof udphdr +  bufferCount);
	ipHeader->ip_tos = 0;
	ipHeader->ip_v = 4;
	ipHeader->ip_p = IPPROTO_UDP;

	//udp
	udphdr_ptr udpHeader = (udphdr_ptr)(((PUCHAR)ipHeader) + ipHeader->ip_hl*4);
	udpHeader->th_dport = htons( desPort );
	udpHeader->th_sport = htons( srcPort );
	udpHeader->length = htons(sizeof udphdr + bufferCount);
	memcpy_s((BYTE*)(udpHeader+1), bufferCount, buffer, bufferCount);
	
	BarbaUtils::RecalculateIPChecksum(ipHeader);
	return sizeof(ether_header) + ntohs(ipHeader->ip_len);
}
