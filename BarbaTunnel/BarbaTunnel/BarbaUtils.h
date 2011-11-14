#pragma once

USHORT ntohs( USHORT netshort );
USHORT htons( USHORT value );

class BarbaUtils
{
public:
	//@param folder buffer with MAX_PATH length
	static void GetModuleFolder(TCHAR* folder);
	static DWORD ConvertStringIp(LPCTSTR pszIp);
	static void PrintIp(DWORD ip);
	static BYTE ConvertStringpProtocol(LPCTSTR protocol);
	static bool GetProtocolAndPort(LPCTSTR value, BYTE* protocol, int* port);
	static u_short CheckSum(USHORT *buffer, int size);
	static void RecalculateIPChecksum(iphdr_ptr pIpHeader, bool calculateProtoCheckSum=true);
	static void RecalculateTCPChecksum(iphdr_ptr ipHeader);
	static void RecalculateTCPChecksum2(iphdr_ptr ipHeader);
	static void RecalculateUDPChecksum(iphdr_ptr ipHeader);
	static void RecalculateICMPChecksum(iphdr_ptr ipHeader);
	static int CreateUdpPacket(BYTE* srcEthAddress, BYTE* desEthAddress, DWORD srcIp, DWORD desIP, u_short srcPort, u_short desPort, BYTE* buffer, size_t bufferCount, ether_header_ptr udpPacket);
};