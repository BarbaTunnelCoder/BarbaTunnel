#include "StdAfx.h"
#include "PacketHelper.h"

void PacketHelper::AllocIpBuffer(size_t ipLen)
{
	ipLen  = min(ipLen, 0xFFFF);
	ipLen = max(ipLen, sizeof iphdr);
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
		memset(buffer, 0, size);
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
	{
		delete this->PacketBuffer;
	}
}

PacketHelper::PacketHelper(PacketHelper* packet)
{
	this->PacketBuffer = new BYTE[packet->PacketSize];
	this->PacketSize = packet->PacketSize;
	memcpy_s(this->PacketBuffer, this->PacketSize, packet->PacketBuffer, this->PacketSize);
	this->ethHeader = (ether_header_ptr)this->PacketBuffer;
	Reinit();
}

PacketHelper::PacketHelper(ether_header_ptr packet, size_t bufferLen)
{
	this->PacketBuffer = NULL;
	this->PacketSize = 0;
	SetEthPacket((ether_header_ptr)packet, bufferLen);
}

PacketHelper::PacketHelper(iphdr_ptr ipHeader, size_t bufferLen)
{
	this->PacketBuffer = NULL;
	this->PacketSize = 0;
	SetIpPacket(ipHeader, bufferLen);
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
	//set minimum IP size
	if (ipProtocol==IPPROTO_TCP) ipLen = max(ipLen, sizeof iphdr + sizeof tcphdr);
	if (ipProtocol==IPPROTO_UDP) ipLen = max(ipLen, sizeof iphdr + sizeof udphdr);

	//allocate default buffer
	AllocIpBuffer(ipLen);
	this->ethHeader->h_proto = htons(ETH_P_IP);
	Reinit();
	this->ipHeader = (iphdr*)(ethHeader + 1);
	this->ipHeader->ip_hl = 5;
	this->ipHeader->ip_p = ipProtocol;
	this->ipHeader->ip_len = htons((u_short)ipLen);
	Reinit();

	if (ipProtocol==IPPROTO_TCP) 
	{
		this->tcpHeader->th_off = 5;
		SetTcpPayload(NULL, 0);
	}
	else if (ipProtocol==IPPROTO_UDP)
	{
		SetUdpPayload(NULL, 0);
	}
}

void PacketHelper::Reinit()
{
	ipHeader = NULL;
	tcpHeader = NULL;
	udpHeader = NULL;

	if (ntohs(ethHeader->h_proto)==ETH_P_IP)
		ipHeader = (iphdr*)(ethHeader + 1);
			
	if (ipHeader!=NULL && ipHeader->ip_p==IPPROTO_TCP)
	{
		tcpHeader = (tcphdr_ptr)(((BYTE*)ipHeader) + ipHeader->ip_hl*4);
	}

	if (ipHeader!=NULL && ipHeader->ip_p==IPPROTO_UDP)
	{
		udpHeader = (udphdr_ptr)(((BYTE*)ipHeader) + ipHeader->ip_hl*4);
	}
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
	AllocIpBuffer(ipLen);

	BYTE* tcpPayload = GetTcpPayload();
	size_t remain = this->PacketSize - (tcpPayload-(BYTE*)ethHeader);
	memcpy_s(tcpPayload, remain, payload, len);
	ipHeader->ip_len = htons( (u_short)ipLen );
}

void PacketHelper::SetUdpPayload(BYTE* payload, size_t len)
{
	size_t ipLen = ipHeader->ip_hl*4 + sizeof(udphdr) + len;
	AllocIpBuffer(ipLen);

	BYTE* udpPayload = GetUdpPayload();
	size_t remain = this->PacketSize - (udpPayload-(BYTE*)ethHeader);
	memcpy_s(udpPayload, remain, payload, len);
	udpHeader->length = htons( (u_short)(sizeof(udphdr) + len) );
	ipHeader->ip_len = htons( (u_short)ipLen );
}

void PacketHelper::RecalculateChecksum()
{
	if (ipHeader!=NULL)
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

void PacketHelper::SetEthHeader(ether_header_ptr ethHeaderParam)
{
	memcpy_s(this->ethHeader, this->PacketSize, ethHeaderParam, sizeof ether_header);
	Reinit();
}

void PacketHelper::SetEthPacket(ether_header_ptr ethHeaderParam, size_t bufferLen)
{
	AllocIpBuffer(bufferLen-sizeof ether_header);
	memcpy_s(this->ethHeader, this->PacketSize, ethHeaderParam, bufferLen);
	Reinit();
}

void PacketHelper::SetIpPacket(iphdr_ptr ipHeaderParam, size_t bufferLen)
{
	AllocIpBuffer(bufferLen);
	this->ethHeader->h_proto = htons(ETH_P_IP);
	Reinit();
	memcpy_s(this->ipHeader, this->PacketSize-sizeof ether_header, ipHeaderParam, bufferLen);
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
	return IsIp() && PacketHelper::IsValidIPChecksum(this->ipHeader, this->PacketSize-sizeof ether_header);
}

bool PacketHelper::IsValidIPChecksum(iphdr_ptr ipHeader, size_t packetSize)
{
	if (ntohs(ipHeader->ip_len)>packetSize)
		return false; //packet length larger than buffer!

	//check UDP integrity
	if (ipHeader->ip_p==IPPROTO_UDP)
	{
		udphdr_ptr udpHeader = (udphdr_ptr)(((BYTE*)ipHeader) + ipHeader->ip_hl*4);
		u_short udpLenCalc = ntohs(ipHeader->ip_len) - ipHeader->ip_hl*4;
		u_short udpLen = ntohs(udpHeader->length);
		if (udpLenCalc!=udpLen)
			return false;
	}

	//check TCP integrity
	if (ipHeader->ip_p==IPPROTO_TCP)
	{
		tcphdr_ptr tcpHeader = (tcphdr_ptr)(((BYTE*)ipHeader) + ipHeader->ip_hl*4);
		int ipLen = ntohs(ipHeader->ip_len);
		int tcpOff = (ipHeader->ip_hl*4 + tcpHeader->th_off*4);
		if ( tcpOff > ipLen )
			return false;
	}

	PacketHelper packet(ipHeader, packetSize);
	PacketHelper::RecalculateIPChecksum(packet.ipHeader, false);
	return packet.ipHeader->ip_sum==ipHeader->ip_sum;
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


void PacketHelper::RecalculateIPChecksum(iphdr_ptr ipHeader, bool calculateProtoCheckSum)
{
	if (calculateProtoCheckSum)
	{
		if (ipHeader->ip_p==IPPROTO_TCP) RecalculateTCPChecksum(ipHeader);
		else if (ipHeader->ip_p==IPPROTO_UDP) RecalculateUDPChecksum(ipHeader);
		else if (ipHeader->ip_p==IPPROTO_ICMP) RecalculateICMPChecksum(ipHeader);
		//else _tprintf_s("Unknown protocol for checksum!\n"); 
	}

	u_int sum = 0;

	// Initialize checksum to zero
	ipHeader->ip_sum = 0;
	BYTE* buff = (BYTE*)ipHeader;

	// Calculate IP header checksum
	u_short word16;
	for (size_t i = 0; i < (size_t)ipHeader->ip_hl*4; i=i+2)
	{
		word16 = ((buff[i]<<8)&0xFF00)+(buff[i+1]&0xFF);
		sum = sum+word16; 
	}

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
	while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);

	// Take the one's complement of sum
	sum = ~sum;

	ipHeader->ip_sum = htons((u_short) sum);
}

void PacketHelper::RecalculateTCPChecksum(iphdr_ptr ipHeader)
{
	u_int sum = 0;

	// Sanity check
	if (ipHeader->ip_p != IPPROTO_TCP)
		return;
	
	tcphdr_ptr tcpHeader = (tcphdr_ptr)(((BYTE*)ipHeader) + ipHeader->ip_hl*4);
	size_t tcpLen = ntohs(ipHeader->ip_len) - ipHeader->ip_hl*4;//pPacket->m_Length - ((BYTE*)(tcpHeader) - pPacket->m_IBuffer);

	BYTE* buff = (BYTE*)tcpHeader;
	tcpHeader->th_sum = 0;

	// make 16 bit words out of every two adjacent 8 bit words and 
	// calculate the sum of all 16 bit words
	u_short word16;
	for (size_t i=0; i< tcpLen; i=i+2)
	{
		word16 = (buff[i]<<8) & 0xFF00;
		if ( i+1<tcpLen ) word16 += buff[i+1] & 0xFF;
		sum = sum + (u_int)word16;
	}
	
	// add the TCP pseudo header which contains:
	// the IP source and destination addresses,

	sum = sum + ntohs(ipHeader->ip_src.S_un.S_un_w.s_w1) + ntohs(ipHeader->ip_src.S_un.S_un_w.s_w2);
	sum = sum + ntohs(ipHeader->ip_dst.S_un.S_un_w.s_w1) + ntohs(ipHeader->ip_dst.S_un.S_un_w.s_w2);
	
	// the protocol number and the length of the TCP packet
	sum = sum + IPPROTO_TCP + (u_short)tcpLen;

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum;

	tcpHeader->th_sum = htons((u_short)sum);
}

void PacketHelper::RecalculateICMPChecksum(iphdr_ptr ipHeader)
{
	u_int sum = 0;
	if (ipHeader->ip_p != IPPROTO_ICMP)
		return;
	
	icmphdr_ptr icmpHeader = (icmphdr_ptr)(((BYTE*)ipHeader) + ipHeader->ip_hl*4);

	size_t icmpLen = ntohs(ipHeader->ip_len) - ipHeader->ip_hl*4;
	BYTE* buff = (BYTE*)icmpHeader;
	icmpHeader->checksum = 0;

	// make 16 bit words out of every two adjacent 8 bit words and 
	// calculate the sum of all 16 bit words
	u_short word16;
	for (size_t i=0; i< icmpLen; i=i+2)
	{
		word16 = (buff[i]<<8) & 0xFF00;
		if ( i+1<icmpLen ) word16 += buff[i+1] & 0xFF;
		sum = sum + (u_int)word16;
	}
	
	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum;

	icmpHeader->checksum = ntohs((u_short)sum);
}

void PacketHelper::RecalculateUDPChecksum(iphdr_ptr ipHeader)
{
	u_int sum = 0;
	if (ipHeader->ip_p != IPPROTO_UDP)
		return;
	
	udphdr_ptr udpHeader = (udphdr_ptr)(((BYTE*)ipHeader) + ipHeader->ip_hl*4);
	size_t udpLen = ntohs(ipHeader->ip_len) - ipHeader->ip_hl*4;//pPacket->m_Length - ((BYTE*)(tcpHeader) - pPacket->m_IBuffer);
	BYTE* buff = (BYTE*)udpHeader;
	udpHeader->th_sum = 0;

	// make 16 bit words out of every two adjacent 8 bit words and 
	// calculate the sum of all 16 bit words
	u_int word16;
	for (size_t i=0; i<udpLen; i=i+2)
	{
		word16 = (buff[i]<<8) & 0xFF00;
		if ( i+1<udpLen ) word16 += buff[i+1] & 0xFF;
		sum = sum + (u_int)word16;
	}
	
	// add the UDP pseudo header which contains:
	// the IP source and destination addresses,

	sum = sum + ntohs(ipHeader->ip_src.S_un.S_un_w.s_w1) + ntohs(ipHeader->ip_src.S_un.S_un_w.s_w2);
	sum = sum + ntohs(ipHeader->ip_dst.S_un.S_un_w.s_w1) + ntohs(ipHeader->ip_dst.S_un.S_un_w.s_w2);
	
	// the protocol number and the length of the UDP packet
	sum = sum + IPPROTO_UDP + (u_short)udpLen;

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum;

	udpHeader->th_sum = ntohs((u_short)sum);
}

DWORD PacketHelper::ConvertStringIp(LPCTSTR pszIp)
{
	return inet_addr(pszIp);
}

void PacketHelper::ConvertIpToString(DWORD ip, TCHAR* buffer, rsize_t bufferCount)
{
	_stprintf_s(buffer, bufferCount, _T("%d.%d.%d.%d"), LOBYTE(LOWORD(ip)), HIBYTE(LOWORD(ip)), LOBYTE(HIWORD(ip)), HIBYTE(HIWORD(ip)));
}

char* PacketHelper::ConvertStringToIp(DWORD ip)
{
	in_addr in;
	in.S_un.S_addr = ip;
	return inet_ntoa(in);
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
	else if (_tcsicmp(protocol, _T("ESP"))==0) return 50;

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
	case 50: return _T("ESP");
	default: return _T("?");
	}
}
