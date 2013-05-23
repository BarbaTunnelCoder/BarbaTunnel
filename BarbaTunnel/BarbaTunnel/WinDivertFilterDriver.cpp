#include "StdAfx.h"
#include "BarbaSocket.h"
#include "BarbaServer\BarbaServerApp.h"
#include "BarbaClient\BarbaClientApp.h"
#include "BarbaUtils.h"
#include "WinDivertFilterDriver.h"
#include "WinDivert\WinDivertApi.h"

typedef LSTATUS (APIENTRY *REGGETVALUEA) (HKEY hkey, LPCSTR lpSubKey, LPCSTR  lpValue, DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData);
typedef LSTATUS (APIENTRY *REGSETKEYVALUEA) (HKEY hKey, LPCSTR lpSubKey, LPCSTR lpValueName, DWORD dwType, LPCVOID  lpData, DWORD cbData);


WinDivertApi gWinDivertApi;
extern BarbaComm* theComm;
void InitWinDivertApi()
{
	if (gWinDivertApi.ModuleHandle!=NULL)
		return;

	REGGETVALUEA pRegGetValue = (REGGETVALUEA)GetProcAddress(GetModuleHandle(_T("Advapi32.dll")), "RegGetValueA");
	REGSETKEYVALUEA pRegSetKeyValue = (REGSETKEYVALUEA)GetProcAddress(GetModuleHandle(_T("Advapi32.dll")), "RegSetKeyValueA");
	if (pRegGetValue==NULL || pRegSetKeyValue==NULL)
		throw _T("Could not find the required procedure! Note: WinDivert does not work on Windows XP!");
		
	//correct WinDivert.sys path in registry
	TCHAR sysPath[MAX_PATH];
	DWORD dataLen = _countof(sysPath);
	LSTATUS res = pRegGetValue(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\WinDivert1.0"), _T("ImagePath"), RRF_RT_ANY, NULL, sysPath, &dataLen);

	if (res == ERROR_SUCCESS)
	{
		TCHAR myPath[MAX_PATH];
		_stprintf_s(myPath, _T("\\??\\%s\\WinDivert.sys"), BarbaUtils::GetModuleFolder().data()); 
		if (_tcsclen(sysPath)!=0 && _tcsicmp(myPath, sysPath)!=0)
		{
			res = pRegSetKeyValue(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\WinDivert1.0"), _T("ImagePath"), REG_SZ, myPath, (DWORD)_tcslen(myPath)) ;
			if (res != ERROR_SUCCESS )
				throw _T("WinDivert.sys path is invalid and could not be fixed!");
			theComm->Log(_T("WinDivert.sys path was invalid and fixed."), false);
		}
	}

	//initialize DivertModule
	TCHAR curDir[MAX_PATH];
	GetCurrentDirectory(_countof(curDir), curDir);
	SetCurrentDirectory( BarbaApp::GetModuleFolder() ); //WinDivert need current directory to install driver perhaps for WdfCoInstaller01009
	HMODULE module = LoadLibrary(_T("WinDivert.dll"));
	SetCurrentDirectory(curDir);
	if (module==NULL) 
		throw _T("Could not load WinDivert.dll module!");
	gWinDivertApi.Init(module);
}

WinDivertFilterDriver::WinDivertFilterDriver(void)
	: RouteFinderPacket(IPPROTO_RAW, sizeof iphdr)
	, SingalPacket(IPPROTO_RAW, sizeof iphdr)
{
	DivertHandle = NULL;
	MainIfIdx = 0;
	MainSubIfIdx = 0;
	FilterIpOnly = false;

	//initialize RouteFinderPacket
	RouteFinderPacket.SetSrcIp( inet_addr("127.0.0.1") );
	RouteFinderPacket.SetDesIp( inet_addr("8.8.8.8") );
	RouteFinderPacket.RecalculateChecksum();

	//initialize SingalPacket
	SingalPacket.SetSrcIp( inet_addr("127.0.0.1") );
	SingalPacket.SetDesIp( inet_addr("127.0.0.2") );
	SingalPacket.RecalculateChecksum();
}

WinDivertFilterDriver::~WinDivertFilterDriver(void)
{
}

DWORD WinDivertFilterDriver::GetMTUDecrement()
{
	return 0;
}

void WinDivertFilterDriver::SetMTUDecrement(DWORD /*value*/)
{
	throw new BarbaException(_T("WinDivert does not support SetMTUDecrement!"));
}

void WinDivertFilterDriver::Initialize()
{
	DWORD winVersion = GetVersion();
	DWORD majorVersion = (DWORD)(LOBYTE(LOWORD(winVersion)));
	if (majorVersion<6)
	{
		BarbaNotify(_T("Error: Your Windows is not supported!\r\nWinDivert requires Windows Vista or later, you should change the FilterDriver."));
		throw new BarbaException(_T("WinDivert requires Windows Vista or later, you should change the FilterDriver!"));
	}


	InitWinDivertApi();

	this->DivertHandle = OpenDivertHandle();
}

void WinDivertFilterDriver::Dispose()
{
	//WARNING: Dispose parent first and wait till its thread closes
	BarbaFilterDriver::Dispose();

	//all resource disposed in Stop
	if (this->DivertHandle!=NULL)
		gWinDivertApi.DivertClose(this->DivertHandle);
	this->DivertHandle = NULL;
}

void WinDivertFilterDriver::Stop()
{
	BarbaFilterDriver::Stop();
	SendPacketWithSocket(&SingalPacket); //stop DivertRecv
}

HANDLE WinDivertFilterDriver::OpenDivertHandle()
{
	TCHAR curDir[MAX_PATH];
	GetCurrentDirectory(_countof(curDir), curDir);
	SetCurrentDirectory( BarbaApp::GetModuleFolder() ); //WinDivert need current directory to install driver perhaps for WdfCoInstaller01009

	//try to solve WinDivert issue at first time load
	HANDLE divertHandle = gWinDivertApi.DivertOpen("false", DIVERT_LAYER_NETWORK, 0, 0);
	if (divertHandle!=INVALID_HANDLE_VALUE)
		gWinDivertApi.DivertClose(divertHandle);

	//apply filter
	std::string filter;
	AddPacketFilter(&filter);
	divertHandle = gWinDivertApi.DivertOpen(filter.data(), DIVERT_LAYER_NETWORK, 0, 0);
	if (divertHandle==INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_PARAMETER)
	{
		//IP-Only filter
		BarbaLog(_T("WinDivert does not accept filter criteria. Please reduce configuration files. BarbaTunnel try to using IP-Filter only!"));
		this->FilterIpOnly = true;
		filter.clear();
		AddPacketFilter(&filter);
		divertHandle = gWinDivertApi.DivertOpen(filter.data(), DIVERT_LAYER_NETWORK, 0, 0);
	}

	//true filter
	if (divertHandle==INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_PARAMETER)
	{
		BarbaLog(_T("WinDivert does not accept filter criteria. Please reduce configuration files. BarbaTunnel try to capture all network packets (slow performance)!"));
		divertHandle = gWinDivertApi.DivertOpen("true", DIVERT_LAYER_NETWORK, 0, 0);
	}

	if (divertHandle==INVALID_HANDLE_VALUE)
	{
		LPCTSTR msg = 
			_T("Failed to open Divert device (%d)!\r\n")
			_T("1) Try to Install BarbaTunnel again.\r\n")
			_T("2) Make sure your windows has been restarted.\r\n")
			_T("3) Make sure your Windows x64 test signing is on: BCDEDIT /SET TESTSIGNING ON\r\n");
		SetCurrentDirectory( curDir );
		throw new BarbaException(msg, GetLastError());
	}

	SetCurrentDirectory( curDir );
	return divertHandle;
}


void WinDivertFilterDriver::StartCaptureLoop()
{
	//Stop() will close handle so need to be re-open
	if (this->DivertHandle==NULL)
		this->DivertHandle = OpenDivertHandle();

	//SendRouteFinderPacket
	SendPacketWithSocket(&RouteFinderPacket);

	// Main capture-modify-inject loop:
	DIVERT_ADDRESS addr;    // Packet address
	BYTE* buffer = new BYTE[0xFFFF];    // Packet buffer
	UINT recvLen;
	while (!IsStopping())
	{
		//it will return error if this->DivertHandle closed
		if (!gWinDivertApi.DivertRecv(this->DivertHandle, buffer, 0xFFFF, &addr, &recvLen))
			continue;


		//Initialize Packet
		bool send = addr.Direction == DIVERT_PACKET_DIRECTION_OUTBOUND;
		PacketHelper* packet = new PacketHelper((iphdr_ptr)buffer, recvLen);

		//user adapter index for any grab packet
		this->MainIfIdx = addr.IfIdx;
		this->MainSubIfIdx = addr.SubIfIdx;

		//don't send my special packets
		if (HasSamePacketTarget(packet, &RouteFinderPacket) || HasSamePacketTarget(packet, &SingalPacket))
		{
			delete packet;
			continue;
		}

		//add packet to queue
		AddPacket(packet, send);
	}

	delete buffer;
}

bool WinDivertFilterDriver::SendPacketToOutbound(PacketHelper* packet)
{
	DIVERT_ADDRESS addr;
	addr.IfIdx = this->MainIfIdx;
	addr.SubIfIdx = this->MainSubIfIdx;
	addr.Direction = DIVERT_PACKET_DIRECTION_OUTBOUND;
	return 
		this->DivertHandle!=NULL && 
		gWinDivertApi.DivertSend(this->DivertHandle, packet->ipHeader, (UINT)packet->GetIpLen(), &addr, NULL)!=FALSE;
}

bool WinDivertFilterDriver::SendPacketToInbound(PacketHelper* packet)
{
	DIVERT_ADDRESS addr;
	addr.IfIdx = this->MainIfIdx;
	addr.SubIfIdx = this->MainSubIfIdx;
	addr.Direction = DIVERT_PACKET_DIRECTION_INBOUND;
	return 
		this->DivertHandle!=NULL && 
		gWinDivertApi.DivertSend(this->DivertHandle, packet->ipHeader, (UINT)packet->GetIpLen(), &addr, NULL)!=FALSE;
}

void WinDivertFilterDriver::CreateRangeFormat(TCHAR* format, LPCSTR fieldName, DWORD start, DWORD end, bool ip)
{
	CHAR szStart[50];
	CHAR szEnd[50];
	if (ip) 
	{
		sprintf_s(szStart, "%s", PacketHelper::ConvertStringToIp(start));
		sprintf_s(szEnd, "%s", PacketHelper::ConvertStringToIp(end));
	}
	else
	{
		sprintf_s(szStart, "%u", start);
		sprintf_s(szEnd, "%u", end);
	}

	if (start==end)
		sprintf_s(format, 1000, "%s==%s", fieldName, szStart);
	else
		sprintf_s(format, 1000, "%s>=%s && %s<=%s", fieldName, szStart, fieldName, szEnd);
}

std::string WinDivertFilterDriver::GetFilter(bool send, u_long srcIpStart, u_long srcIpEnd, u_long desIpStart, u_long desIpEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd)
{
	srcIpEnd = max(srcIpStart, srcIpEnd);
	desIpEnd = max(desIpStart, desIpEnd);
	srcPortEnd = max(srcPortStart, srcPortEnd);
	desPortEnd = max(desPortStart, desPortEnd);

	CHAR buf[1000];

	std::string filter;
	filter.append( send ? "outbound" : "inbound" );

	//IP filter
	if (srcIpStart!=0)
	{
		CreateRangeFormat(buf, "ip.SrcAddr", srcIpStart, srcIpEnd, true);
		filter.append(" && ");
		filter.append(buf);
	}

	if (desIpStart!=0)
	{
		CreateRangeFormat(buf, "ip.DstAddr", desIpStart, desIpEnd, true);
		filter.append(" && ");
		filter.append(buf);
	}

	//stop in FilterIpOnly mode
	if (this->FilterIpOnly && (srcIpStart!=0 || desIpStart!=0))
		return filter;

	//protocol filter
	if (protocol!=0)
	{
		sprintf_s(buf, "ip.Protocol==%u", protocol);
		filter.append(" && ");
		filter.append(buf);
	}

	//TCP source port filter
	if (srcPortStart!=0 && protocol==IPPROTO_TCP)
	{
		CreateRangeFormat(buf, "tcp.SrcPort", srcPortStart, srcPortEnd);
		filter.append(" && ");
		filter.append(buf);
	}

	//UDP source port filter
	if (srcPortStart!=0 && protocol==IPPROTO_UDP)
	{
		CreateRangeFormat(buf, "udp.SrcPort", srcPortStart, srcPortEnd);
		filter.append(" && ");
		filter.append(buf);
	}

	//TCP destination port filter
	if (desPortStart!=0 && protocol==IPPROTO_TCP)
	{
		CreateRangeFormat(buf, "tcp.DstPort", desPortStart, desPortEnd);
		filter.append(" && ");
		filter.append(buf);
	}

	//source port filter
	if (desPortStart!=0 && protocol==IPPROTO_UDP)
	{
		CreateRangeFormat(buf, "udp.DstPort", desPortStart, desPortEnd);
		filter.append(" && ");
		filter.append(buf);
	}

	return filter;
}

void WinDivertFilterDriver::AddFilter(void* pfilter, bool send, u_long srcIpStart, u_long srcIpEnd, u_long desIpStart, u_long desIpEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd)
{
	std::string* filter = (std::string*)pfilter;
	std::string res = GetFilter(send, srcIpStart, srcIpEnd, desIpStart, desIpEnd, protocol, srcPortStart, srcPortEnd, desPortStart, desPortEnd);
	if (!filter->empty())
		filter->append(" || ");

	filter->append("(");
	filter->append(res);
	filter->append(")");
}

void WinDivertFilterDriver::AddPacketFilter(void* filter)
{
	BarbaFilterDriver::AddPacketFilter(filter);

	//add route finder filter & singal packet
	AddFilter(filter, true, 0, 0, RouteFinderPacket.GetDesIp(), 0, RouteFinderPacket.ipHeader->ip_p, 0, 0, 0, 0);
	AddFilter(filter, true, 0, 0, SingalPacket.GetDesIp(), 0, SingalPacket.ipHeader->ip_p, 0, 0, 0, 0);
}

bool WinDivertFilterDriver::HasSamePacketTarget( PacketHelper* packet1,  PacketHelper* packet2)
{
	return 
		packet1->ipHeader->ip_p == packet2->ipHeader->ip_p &&
		packet1->GetDesIp() == packet2->GetDesIp() &&
		packet1->GetDesPort() == packet2->GetDesPort();
}


