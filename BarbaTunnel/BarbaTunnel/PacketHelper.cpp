#include "StdAfx.h"
#include "PacketHelper.h"

void PacketHelper::AllocIpBuffer(size_t ipLen)
{
	if (ipLen>0xFFFF)
		throw _T("Invalid IP size!");

	size_t size = ipLen + sizeof ether_header;

	//allocate new buffer
	if (this->PacketBuffer==NULL)
	{
		this->PacketSize = size;
		this->PacketBuffer = new BYTE[size];
		memset(this->PacketBuffer, 0, size);
	}

	//reallocate buffer
	else if (this->PacketSize<size)
	{
		BYTE* buffer = new BYTE[size];
		memcpy_s(buffer, size, this->PacketBuffer, this->PacketSize);
		delete this->PacketBuffer;
		this->PacketSize = size;
		this->PacketBuffer = buffer;
	}
	else
	{
		//buffer have enough size
	}

	this->ethHeader = (ether_header_ptr)this->PacketBuffer;
	Reinit();
}

PacketHelper::~PacketHelper()
{
	if (this->PacketBuffer!=NULL)
		delete this->PacketBuffer;
}

PacketHelper::PacketHelper(ether_header_ptr packet)
{
	this->PacketBuffer = NULL;
	this->PacketSize = 0;
	SetEthPacket((ether_header_ptr)packet);
}

PacketHelper::PacketHelper(iphdr_ptr ipHeader)
{
	this->PacketBuffer = NULL;
	this->PacketSize = 0;
	SetIpPacket(ipHeader);
}

PacketHelper::PacketHelper()
{
	this->PacketBuffer = NULL;
	this->PacketSize = 0;
	this->Reset(IPPROTO_IP, 0xFFFF);
}

PacketHelper::PacketHelper(size_t ipLen)
{
	this->PacketBuffer = NULL;
	this->PacketSize = 0;
	this->Reset(IPPROTO_IP, ipLen);
}

PacketHelper::PacketHelper(u_char ipProtocol, size_t ipLen)
{
	this->PacketBuffer = NULL;
	this->PacketSize = 0;
	this->Reset(ipProtocol, ipLen);
}

void PacketHelper::Reset(u_char ipProtocol, size_t ipLen)
{
	AllocIpBuffer(ipLen);
	this->ethHeader->h_proto = htons(ETH_P_IP);
	Reinit();
	this->ipHeader = (iphdr*)(ethHeader + 1);
	this->ipHeader->ip_hl = 5;
	this->ipHeader->ip_p = ipProtocol;
	this->ipHeader->ip_len = htons((u_short)ipLen);
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
	size_t ipLen = ipHeader->ip_hl*4 + tcpHeader->th_off*4 + len;
	if (ipLen>0xFFFF)
		throw _T("Invalid IP size!");
	AllocIpBuffer(ipLen);

	BYTE* tcpPayload = GetTcpPayload();
	size_t remain = this->PacketSize - (tcpPayload-(BYTE*)ethHeader);
	memcpy_s(tcpPayload, remain, payload, len);
	ipHeader->ip_len = htons( (u_short)ipLen );
}

void PacketHelper::SetUdpPayload(BYTE* payload, size_t len)
{
	size_t ipLen = ipHeader->ip_hl*4 + sizeof(udphdr) + len;
	if (ipLen>0xFFFF)
		throw _T("Invalid IP size!");
	AllocIpBuffer(ipLen);

	BYTE* udpPayload = GetUdpPayload();
	size_t remain = this->PacketSize - (udpPayload-(BYTE*)ethHeader);
	memcpy_s(udpPayload, remain, payload, len);
	udpHeader->length = htons( (u_short)(sizeof(udphdr) + len) );
	ipHeader->ip_len = htons( (u_short)ipLen );
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
	
	if (len>MAX_ETHER_FRAME)
		throw _T("Invalid Ethernet packet size!");
	AllocIpBuffer(MAX_ETHER_FRAME);
	memcpy_s(this->ethHeader, MAX_ETHER_FRAME, ethHeader, len);
	Reinit();
}

void PacketHelper::SetIpPacket(iphdr_ptr ipHeader)
{
	size_t ipLen = ntohs(ipHeader->ip_len);
	AllocIpBuffer(ipLen);
	this->ethHeader->h_proto = htons(ETH_P_IP);
	Reinit();
	memcpy_s(this->ipHeader, this->PacketSize-sizeof ether_header, ipHeader, ipLen);
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
	PacketHelper packet(ipHeader);
	PacketHelper::RecalculateIPChecksum(packet.ipHeader, false);
	return packet.ipHeader->ip_sum==ipHeader->ip_sum;
}


void PacketHelper::RecalculateIPChecksum(iphdr_ptr pIpHeader, bool calculateProtoCheckSum)
{
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

	pIpHeader->ip_sum = htons((u_short) sum);
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
		sum = sum + (u_int)word16;
	}
	
	// add the TCP pseudo header which contains:
	// the IP source and destination addresses,

	sum = sum + ntohs(pIpHeader->ip_src.S_un.S_un_w.s_w1) + ntohs(pIpHeader->ip_src.S_un.S_un_w.s_w2);
	sum = sum + ntohs(pIpHeader->ip_dst.S_un.S_un_w.s_w1) + ntohs(pIpHeader->ip_dst.S_un.S_un_w.s_w2);
	
	// the protocol number and the length of the TCP packet
	sum = sum + IPPROTO_TCP + (u_short)dwTcpLen;

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum;

	pTcpHeader->th_sum = htons((u_short)sum);
}

void PacketHelper::RecalculateICMPChecksum(iphdr_ptr pIpHeader)
{
	u_short word16, padd = 0;
	u_int i, sum = 0;
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
		sum = sum + (u_int)word16;
	}
	
	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum;

	pIcmpHeader->checksum = ntohs((u_short)sum);
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
		sum = sum + (u_int)word16;
	}
	
	// add the UDP pseudo header which contains:
	// the IP source and destination addresses,

	sum = sum + ntohs(pIpHeader->ip_src.S_un.S_un_w.s_w1) + ntohs(pIpHeader->ip_src.S_un.S_un_w.s_w2);
	sum = sum + ntohs(pIpHeader->ip_dst.S_un.S_un_w.s_w1) + ntohs(pIpHeader->ip_dst.S_un.S_un_w.s_w2);
	
	// the protocol number and the length of the UDP packet
	sum = sum + IPPROTO_UDP + (u_short)dwUdpLen;

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum;

	pUdpHeader->th_sum = ntohs((u_short)sum);
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
