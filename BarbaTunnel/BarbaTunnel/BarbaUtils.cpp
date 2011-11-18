#include "stdafx.h"
#include "BarbaUtils.h"
#include "PacketHelper.h"

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

bool BarbaUtils::GetPortRange(LPCTSTR value, u_short* startPort, u_short* endPort)
{
	//VirtualIpRange
	TCHAR* dash = _tcschr((TCHAR*)value, '-');
	
	TCHAR ipBuffer[100];
	_tcsncpy_s(ipBuffer, _countof(ipBuffer), value, dash!=NULL ? dash-value : _tcslen(value));
	*startPort = (u_short)_tcstoul(ipBuffer, NULL, 0);
	*endPort = *startPort; //default
	if (dash!=NULL)
	{
		_tcscpy_s(ipBuffer, _countof(ipBuffer), dash+1);
		*endPort = (u_short)_tcstoul(ipBuffer, NULL, 0);
	}


	return *startPort!=0;
}

bool BarbaUtils::GetProtocolAndPort(LPCTSTR value, BYTE* protocol, u_short* port)
{
	bool ret = false;
	*protocol = 0;
	*port = 0;
	TCHAR buffer[100];
	_tcscpy_s(buffer, value);

	TCHAR* currentPos = NULL;
	TCHAR* token = _tcstok_s(buffer, _T(":"), &currentPos);
		
	int index = 0;
	while (token!=NULL && index<2)
	{
		if (index==0 && _tcsicmp(token, _T("*"))==0) {*protocol = 0; ret = true;}
		else if (index==0) {*protocol = PacketHelper::ConvertStringProtocol(token); ret = *protocol!=0;}
		if (index==1) *port = (u_short)_tcstoul(token, NULL, 0);
		token = _tcstok_s(NULL, _T("."), &currentPos);
		index++;
	}

	return ret;
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
	ipHeader->ip_len = htons( (u_short)( sizeof iphdr + sizeof udphdr +  bufferCount) );
	ipHeader->ip_tos = 0;
	ipHeader->ip_v = 4;
	ipHeader->ip_p = IPPROTO_UDP;

	//udp
	udphdr_ptr udpHeader = (udphdr_ptr)(((PUCHAR)ipHeader) + ipHeader->ip_hl*4);
	udpHeader->th_dport = htons( desPort );
	udpHeader->th_sport = htons( srcPort );
	udpHeader->length = htons( (u_short)(sizeof udphdr + bufferCount));
	memcpy_s((BYTE*)(udpHeader+1), bufferCount, buffer, bufferCount);
	
	PacketHelper::RecalculateIPChecksum(ipHeader);
	return sizeof(ether_header) + ntohs(ipHeader->ip_len);
}


int BarbaUtils::ConvertHexStringToBuffer(TCHAR* hexString, BYTE* buffer, int bufferCount)
{
	int bufferIndex = 0;

	int len = _tcslen(hexString);
	for(int i=0; ; i+=2)
	{
		if (i>=len || bufferIndex>=bufferCount)
			break;
		char b[3] = {0};
		b[0] = hexString[i];
		b[1] = (i+1<len) ? hexString[i+1] : 0;
		b[2] = 0;
		buffer[bufferIndex++] = (BYTE)strtol(b, NULL, 16);
	}

	return bufferIndex;
}
