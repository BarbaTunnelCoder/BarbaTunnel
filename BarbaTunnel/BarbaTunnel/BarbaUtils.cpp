#include "stdafx.h"
#include "General.h"
#include "BarbaUtils.h"

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


	return *startPort!=0 && (*endPort-*startPort)>=0;
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


void BarbaUtils::ConvertHexStringToBuffer(LPCTSTR hexString, SimpleBuffer* buf)
{
	int len = _tcslen(hexString);
	buf->New(len/2);

	int bufferIndex = 0;
	for(int i=0; i<(int)buf->GetSize(); i+=2)
	{
		char b[3];
		b[0] = hexString[i];
		b[1] = hexString[i+1];
		b[2] = 0;
		buf->GetData()[bufferIndex++] = (BYTE)strtol(b, NULL, 16);
	}
}

bool BarbaUtils::SimpleShellExecuteAndWait(LPCTSTR fileName, LPCTSTR commandLine, int nShow, LPCTSTR lpszWorkDirectory, LPCTSTR lpVerb, HWND hWnd)
{
	DWORD exitCode;
	return SimpleShellExecute(fileName, commandLine, nShow, lpszWorkDirectory, lpVerb, hWnd, &exitCode);
}


bool BarbaUtils::SimpleShellExecute(LPCTSTR fileName, LPCTSTR commandLine, int nShow, LPCTSTR lpszWorkDirectory, LPCTSTR lpVerb, HWND hWnd, DWORD* lpExitCode)
{
	SHELLEXECUTEINFO s;
	memset (&s,0,sizeof s);

	s.cbSize = sizeof SHELLEXECUTEINFO;
	s.fMask = (lpExitCode!=NULL) ? SEE_MASK_NOCLOSEPROCESS : 0;
	s.hwnd = hWnd;
	s.lpVerb = lpVerb;
	s.lpFile = fileName;
	s.lpParameters = commandLine;
	s.lpDirectory = lpszWorkDirectory;
	s.nShow = nShow;
	bool ret = ShellExecuteEx(&s) != FALSE;

	if (lpExitCode!=NULL)
	{
		for(;;)
		{
			WaitForSingleObject(s.hProcess, INFINITE);
			GetExitCodeProcess(s.hProcess, lpExitCode);
			if ( *lpExitCode!=STILL_ACTIVE )
				break;
		}
	}

	//close process handle
	if (s.hProcess!=NULL)
		CloseHandle(s.hProcess);

	return ret;
}

size_t BarbaUtils::ParsePortRanges(LPCTSTR value, PortRange* portRanges, size_t portRangeCount)
{
	size_t count = 0;
	
	TCHAR buffer[1000];
	_tcscpy_s(buffer, value);
	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(buffer, _T(","), &currentPos);
		
	while (token!=NULL && count<portRangeCount)
	{
		PortRange* portRange = &portRanges[count];
		if (BarbaUtils::GetPortRange(token, &portRange->StartPort, &portRange->EndPort))
		{
			count++;
		}
		token = _tcstok_s(NULL, _T(","), &currentPos);
	}

	return count;

}

bool BarbaUtils::IsThreadAlive(const HANDLE hThread, bool* alive)
{
	DWORD dwExitCode = 0;
	if (!GetExitCodeThread(hThread, &dwExitCode))
		return false;
	*alive = dwExitCode==STILL_ACTIVE;
	return true;
}

bool BarbaUtils::LoadFileToBuffer(LPCTSTR fileName, SimpleBuffer* buffer)
{
	bool ret = false;

	FILE* f;
	if (_tfopen_s(&f, fileName, _T("rb"))!=0)
		return false;

	fseek(f, 0, SEEK_END);
	size_t fileSize = ftell(f); 
	fseek(f, 0, SEEK_SET);

	buffer->New(fileSize);
	ret = fread_s(buffer->GetData(), buffer->GetSize(), 1, fileSize, f)==fileSize;
	fclose(f);
	return ret;
}

std::string BarbaUtils::LoadFileToString(LPCTSTR fileName)
{
	std::ifstream ifs(fileName);
	std::string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	return str;
}

u_int BarbaUtils::GetRandom(u_int start, u_int end)
{
	u_int range = end - start + 1;

	u_int number;
	if ( rand_s(&number) )
		_tprintf_s(_T("rand_s return error!\n"));

	return (u_int)((double)number / ((double) UINT_MAX + 1 ) * range) + start;
}