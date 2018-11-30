#pragma once
#include "ndisapi.h"

class WinpkFilterApi
{
public:
	HMODULE ModuleHandle;
	typedef HANDLE (__stdcall* OPENFILTERDRIVER) ( const TCHAR* pszFileName); 
	typedef VOID (__stdcall* CLOSEFILTERDRIVER) ( HANDLE hOpen );
	typedef DWORD (__stdcall* GETDRIVERVERSION) ( HANDLE hOpen );
	typedef BOOL (__stdcall* GETTCPIPBOUNDADAPTERSINFO) ( HANDLE hOpen, PTCP_AdapterList pAdapters );
	typedef BOOL (__stdcall* SENDPACKETTOMSTCP) ( HANDLE hOpen, PETH_REQUEST pPacket );
	typedef BOOL (__stdcall* SENDPACKETTOADAPTER) ( HANDLE hOpen, PETH_REQUEST pPacket );
	typedef BOOL (__stdcall* READPACKET) ( HANDLE hOpen, PETH_REQUEST pPacket );
	typedef BOOL (__stdcall* SENDPACKETSTOMSTCP) ( HANDLE hOpen, PETH_M_REQUEST pPackets );
	typedef BOOL (__stdcall* SENDPACKETSTOADAPTER) ( HANDLE hOpen, PETH_M_REQUEST pPackets );
	typedef BOOL (__stdcall* READPACKETS) ( HANDLE hOpen, PETH_M_REQUEST pPackets );
	typedef BOOL (__stdcall* SETADAPTERMODE) ( HANDLE hOpen, PADAPTER_MODE pMode );
	typedef BOOL (__stdcall* GETADAPTERMODE) ( HANDLE hOpen, PADAPTER_MODE pMode );
	typedef BOOL (__stdcall* FLUSHADAPTERPACKETQUEUE) ( HANDLE hOpen, HANDLE hAdapter );
	typedef BOOL (__stdcall* GETADAPTERPACKETQUEUESIZE) ( HANDLE hOpen, HANDLE hAdapter , PDWORD pdwSize);
	typedef BOOL (__stdcall* SETPACKETEVENT) ( HANDLE hOpen, HANDLE hAdapter, HANDLE hWin32Event );
	typedef BOOL (__stdcall* SETWANEVENT) ( HANDLE hOpen, HANDLE hWin32Event );
	typedef BOOL (__stdcall* SETADAPTERLISTCHANGEEVENT) ( HANDLE hOpen, HANDLE hWin32Event );
	typedef BOOL (__stdcall* NDISRDREQUEST) ( HANDLE hOpen, PPACKET_OID_DATA OidData, BOOL Set );
	typedef BOOL (__stdcall* GETRASLINKS) ( HANDLE hOpen, HANDLE hAdapter, PRAS_LINKS pLinks);
	typedef BOOL (__stdcall* SETHWPACKETFILTER) ( HANDLE hOpen, HANDLE hAdapter, DWORD Filter );
	typedef BOOL (__stdcall* GETHWPACKETFILTER) ( HANDLE hOpen, HANDLE hAdapter, PDWORD pFilter );
	typedef BOOL (__stdcall* SETPACKETFILTERTABLE) ( HANDLE hOpen, PSTATIC_FILTER_TABLE pFilterList );
	typedef BOOL (__stdcall* RESETPACKETFILTERTABLE) ( HANDLE hOpen );
	typedef BOOL (__stdcall* GETPACKETFILTERTABLESIZE) ( HANDLE hOpen, PDWORD pdwTableSize );
	typedef BOOL (__stdcall* GETPACKETFILTERTABLE) ( HANDLE hOpen, PSTATIC_FILTER_TABLE pFilterList );
	typedef BOOL (__stdcall* GETPACKETFILTERTABLERESETSTATS) ( HANDLE hOpen, PSTATIC_FILTER_TABLE pFilterList );
	typedef BOOL (__stdcall* SETMTUDECREMENT) ( DWORD dwMTUDecrement );
	typedef DWORD (__stdcall* GETMTUDECREMENT) ();
	typedef BOOL (__stdcall* SETADAPTERSSTARTUPMODE) ( DWORD dwStartupMode );
	typedef DWORD (__stdcall* GETADAPTERSSTARTUPMODE) ();
	typedef BOOL (__stdcall* ISDRIVERLOADED) ( HANDLE hOpen );
	typedef DWORD (__stdcall* GETBYTESRETURNED) ( HANDLE hOpen );
	typedef BOOL (__stdcall* CONVERTWINDOWSNTADAPTERNAME) (LPCSTR szAdapterName,LPSTR szUserFriendlyName,	DWORD dwUserFriendlyNameLength);
	typedef BOOL (__stdcall* CONVERTWINDOWS2000ADAPTERNAME) (LPCSTR szAdapterName,LPSTR szUserFriendlyName,DWORD dwUserFriendlyNameLength);
	typedef BOOL (__stdcall* CONVERTWINDOWS9XADAPTERNAME) (LPCSTR szAdapterName,LPSTR szUserFriendlyName,DWORD dwUserFriendlyNameLength);	
	
	OPENFILTERDRIVER OpenFilterDriver;
	CLOSEFILTERDRIVER CloseFilterDriver;
	GETDRIVERVERSION GetDriverVersion;
	GETTCPIPBOUNDADAPTERSINFO GetTcpipBoundAdaptersInfo;
	SENDPACKETTOMSTCP SendPacketToMstcp;
	SENDPACKETTOADAPTER SendPacketToAdapter;
	READPACKET ReadPacket;
	SENDPACKETSTOMSTCP SendPacketsToMstcp;
	SENDPACKETSTOADAPTER SendPacketsToAdapter;
	READPACKETS ReadPackets;
	SETADAPTERMODE SetAdapterMode;
	GETADAPTERMODE GetAdapterMode;
	FLUSHADAPTERPACKETQUEUE FlushAdapterPacketQueue;
	GETADAPTERPACKETQUEUESIZE GetAdapterPacketQueueSize;
	SETPACKETEVENT SetPacketEvent;
	SETWANEVENT SetWANEvent;
	SETADAPTERLISTCHANGEEVENT SetAdapterListChangeEvent;
	NDISRDREQUEST NdisrdRequest;
	GETRASLINKS GetRasLinks;
	SETHWPACKETFILTER SetHwPacketFilter;
	GETHWPACKETFILTER GetHwPacketFilter;
	SETPACKETFILTERTABLE SetPacketFilterTable;
	RESETPACKETFILTERTABLE ResetPacketFilterTable;
	GETPACKETFILTERTABLESIZE GetPacketFilterTableSize;
	GETPACKETFILTERTABLE GetPacketFilterTable;
	GETPACKETFILTERTABLERESETSTATS GetPacketFilterTableResetStats;
	SETMTUDECREMENT SetMTUDecrement;
	GETMTUDECREMENT GetMTUDecrement;
	SETADAPTERSSTARTUPMODE SetAdaptersStartupMode;
	GETADAPTERSSTARTUPMODE GetAdaptersStartupMode;
	ISDRIVERLOADED IsDriverLoaded;
	GETBYTESRETURNED GetBytesReturned;
	CONVERTWINDOWSNTADAPTERNAME ConvertWindowsNTAdapterName;
	CONVERTWINDOWS2000ADAPTERNAME ConvertWindows2000AdapterName;
	CONVERTWINDOWS9XADAPTERNAME ConvertWindows9xAdapterName;

	WinpkFilterApi()
	{
		this->ModuleHandle = NULL;
	}

	void Init(HMODULE moudle)
	{
		this->ModuleHandle = moudle;
		this->OpenFilterDriver = (OPENFILTERDRIVER)GetFunction("OpenFilterDriver");
		this->CloseFilterDriver = (CLOSEFILTERDRIVER)GetFunction("CloseFilterDriver");
		this->GetDriverVersion = (GETDRIVERVERSION)GetFunction("GetDriverVersion");
		this->GetTcpipBoundAdaptersInfo = (GETTCPIPBOUNDADAPTERSINFO)GetFunction("GetTcpipBoundAdaptersInfo");
		this->SendPacketToMstcp = (SENDPACKETTOMSTCP)GetFunction("SendPacketToMstcp");
		this->SendPacketToAdapter = (SENDPACKETTOADAPTER)GetFunction("SendPacketToAdapter");
		this->ReadPacket = (READPACKET)GetFunction("ReadPacket");
		this->SendPacketsToMstcp = (SENDPACKETSTOMSTCP)GetFunction("SendPacketsToMstcp");
		this->SendPacketsToAdapter = (SENDPACKETSTOADAPTER)GetFunction("SendPacketsToAdapter");
		this->ReadPackets = (READPACKETS)GetFunction("ReadPackets");
		this->SetAdapterMode = (SETADAPTERMODE)GetFunction("SetAdapterMode");
		this->GetAdapterMode = (GETADAPTERMODE)GetFunction("GetAdapterMode");
		this->FlushAdapterPacketQueue = (FLUSHADAPTERPACKETQUEUE)GetFunction("FlushAdapterPacketQueue");
		this->GetAdapterPacketQueueSize = (GETADAPTERPACKETQUEUESIZE)GetFunction("GetAdapterPacketQueueSize");
		this->SetPacketEvent = (SETPACKETEVENT)GetFunction("SetPacketEvent");
		this->SetWANEvent = (SETWANEVENT)GetFunction("SetWANEvent");
		this->SetAdapterListChangeEvent = (SETADAPTERLISTCHANGEEVENT)GetFunction("SetAdapterListChangeEvent");
		this->NdisrdRequest = (NDISRDREQUEST)GetFunction("NdisrdRequest");
		this->GetRasLinks = (GETRASLINKS)GetFunction("GetRasLinks");
		this->SetHwPacketFilter = (SETHWPACKETFILTER)GetFunction("SetHwPacketFilter");
		this->GetHwPacketFilter = (GETHWPACKETFILTER)GetFunction("GetHwPacketFilter");
		this->SetPacketFilterTable = (SETPACKETFILTERTABLE)GetFunction("SetPacketFilterTable");
		this->ResetPacketFilterTable = (RESETPACKETFILTERTABLE)GetFunction("ResetPacketFilterTable");
		this->GetPacketFilterTableSize = (GETPACKETFILTERTABLESIZE)GetFunction("GetPacketFilterTableSize");
		this->GetPacketFilterTable = (GETPACKETFILTERTABLE)GetFunction("GetPacketFilterTable");
		this->GetPacketFilterTableResetStats = (GETPACKETFILTERTABLERESETSTATS)GetFunction("GetPacketFilterTableResetStats");
		this->SetMTUDecrement = (SETMTUDECREMENT)GetFunction("SetMTUDecrement");
		this->GetMTUDecrement = (GETMTUDECREMENT)GetFunction("GetMTUDecrement");
		this->SetAdaptersStartupMode = (SETADAPTERSSTARTUPMODE)GetFunction("SetAdaptersStartupMode");
		this->GetAdaptersStartupMode = (GETADAPTERSSTARTUPMODE)GetFunction("GetAdaptersStartupMode");
		this->IsDriverLoaded = (ISDRIVERLOADED)GetFunction("IsDriverLoaded");
		this->GetBytesReturned = (GETBYTESRETURNED)GetFunction("GetBytesReturned");
		this->ConvertWindowsNTAdapterName = (CONVERTWINDOWSNTADAPTERNAME)GetFunction("ConvertWindowsNTAdapterName");
		this->ConvertWindows2000AdapterName = (CONVERTWINDOWS2000ADAPTERNAME)GetFunction("ConvertWindows2000AdapterName");
		this->ConvertWindows9xAdapterName = (CONVERTWINDOWS9XADAPTERNAME)GetFunction("ConvertWindows9xAdapterName");
	}

private:
	FARPROC GetFunction(LPCSTR procName)
	{
		FARPROC ret = GetProcAddress(ModuleHandle, procName);
		if (procName==NULL) 
			throw _T("Could not find of the WinpkFilter Functions!");
		return ret;
	}
};
