#pragma once

class BarbaUtils
{
public:
	//@param folder buffer with MAX_PATH length
	static void GetModuleFolder(TCHAR* folder);
	static bool GetPortRange(LPCTSTR value, u_short* startPort, u_short* endPort);
	static bool GetProtocolAndPort(LPCTSTR value, BYTE* protocol, u_short* port);
	static int CreateUdpPacket(BYTE* srcEthAddress, BYTE* desEthAddress, DWORD srcIp, DWORD desIP, u_short srcPort, u_short desPort, BYTE* buffer, size_t bufferCount, ether_header_ptr udpPacket);
	//@return number of bytes copied to buffer
	static int ConvertHexStringToBuffer(TCHAR* hexString, BYTE* buffer, int bufferCount);
	// @param lphProcess return handle to opened process; if not NULL user must close handle after use it
	static bool SimpleShellExecute(LPCTSTR fileName, LPCTSTR commandLine=_T(""), int nShow=SW_SHOWNORMAL, LPCTSTR lpszWorkDirectory = NULL, LPCTSTR lpVerb=NULL, HWND hWnd=NULL);

};