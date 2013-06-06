/*************************************************************************/
/*				Copyright (c) 2000-2013 NT Kernel Resources.		     */
/*                           All Rights Reserved.                        */
/*                          http://www.ntkernel.com                      */
/*                           ndisrd@ntkernel.com                         */
/*																		 */
/* Description: API exported C++ class declaration						 */
/*************************************************************************/

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NDISAPI_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NDISAPI_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef NDISAPI_EXPORTS
#define NDISAPI_API __declspec(dllexport)
#else
#define NDISAPI_API __declspec(dllimport)
#endif

enum
{
	FILE_NAME_SIZE = 1000
};

typedef BOOL (__stdcall *IsWow64ProcessPtr)(HANDLE hProcess, PBOOL Wow64Process);

// This class is exported from the ndisapi.dll
class NDISAPI_API CNdisApi 
{
public:
	CNdisApi (const TCHAR* pszFileName = _T(DRIVER_NAME_A));
	virtual ~CNdisApi ();

	// Public member functions
	BOOL	DeviceIoControl (DWORD dwService, void *BuffIn, int SizeIn, void *BuffOut, int SizeOut, unsigned long *SizeRet = NULL, LPOVERLAPPED povlp = NULL);

	// Driver services
	ULONG	GetVersion ();
	BOOL	GetTcpipBoundAdaptersInfo ( PTCP_AdapterList pAdapters );
	BOOL	SendPacketToMstcp ( PETH_REQUEST pPacket );
	BOOL	SendPacketToAdapter ( PETH_REQUEST pPacket );
	BOOL	ReadPacket ( PETH_REQUEST pPacket );
	BOOL	SendPacketsToMstcp (PETH_M_REQUEST pPackets);
	BOOL	SendPacketsToAdapter(PETH_M_REQUEST pPackets);
	BOOL	ReadPackets(PETH_M_REQUEST pPackets);
	BOOL	SetAdapterMode ( PADAPTER_MODE pMode );
	BOOL	GetAdapterMode ( PADAPTER_MODE pMode );
	BOOL	FlushAdapterPacketQueue ( HANDLE hAdapter );
	BOOL	GetAdapterPacketQueueSize ( HANDLE hAdapter, PDWORD pdwSize );
	BOOL	SetPacketEvent ( HANDLE hAdapter, HANDLE hWin32Event );
	BOOL	SetWANEvent ( HANDLE hWin32Event );
	BOOL	SetAdapterListChangeEvent ( HANDLE hWin32Event );
	BOOL	NdisrdRequest ( PPACKET_OID_DATA OidData, BOOL Set );
	BOOL	GetRasLinks (HANDLE hAdapter, PRAS_LINKS pLinks);
	BOOL	SetHwPacketFilter ( HANDLE hAdapter, DWORD Filter );
	BOOL	GetHwPacketFilter ( HANDLE hAdapter, PDWORD pFilter );
	BOOL	SetPacketFilterTable (PSTATIC_FILTER_TABLE pFilterList );
	BOOL	ResetPacketFilterTable ();
	BOOL	GetPacketFilterTableSize ( PDWORD pdwTableSize );
	BOOL	GetPacketFilterTable ( PSTATIC_FILTER_TABLE pFilterList );
	BOOL	GetPacketFilterTableResetStats ( PSTATIC_FILTER_TABLE pFilterList );
	
	static BOOL		SetMTUDecrement ( DWORD dwMTUDecrement );
	static DWORD	GetMTUDecrement ();

	static BOOL		SetAdaptersStartupMode ( DWORD dwStartupMode );
	static DWORD	GetAdaptersStartupMode ();

	BOOL	IsDriverLoaded ();
	DWORD	GetBytesReturned ();

	// Helper routines
	static BOOL
		ConvertWindowsNTAdapterName (
			LPCSTR szAdapterName,
			LPSTR szUserFriendlyName,
			DWORD dwUserFriendlyNameLength
			);

	static BOOL
		ConvertWindows2000AdapterName (
			LPCSTR szAdapterName,
			LPSTR szUserFriendlyName,
			DWORD dwUserFriendlyNameLength
			);

	static BOOL
		ConvertWindows9xAdapterName (
			LPCSTR szAdapterName,
			LPSTR szUserFriendlyName,
			DWORD dwUserFriendlyNameLength
			);

private:
	// Private member variables
	OVERLAPPED				m_ovlp;
	HANDLE					m_hFileHandle;
	DWORD					m_BytesReturned;
	BOOL					m_bIsLoadSuccessfully;
	OSVERSIONINFO			m_Version;
	IsWow64ProcessPtr		m_pfnIsWow64Process;
	BOOL					m_bIsWow64Process;
	TCP_AdapterList_WOW64	m_AdaptersList;
};

extern "C"
{

HANDLE	__stdcall		OpenFilterDriver ( const TCHAR* pszFileName = _T(DRIVER_NAME_A) ); 
VOID	__stdcall		CloseFilterDriver ( HANDLE hOpen );
DWORD	__stdcall		GetDriverVersion ( HANDLE hOpen );
BOOL	__stdcall		GetTcpipBoundAdaptersInfo ( HANDLE hOpen, PTCP_AdapterList pAdapters );
BOOL	__stdcall		SendPacketToMstcp ( HANDLE hOpen, PETH_REQUEST pPacket );
BOOL	__stdcall		SendPacketToAdapter ( HANDLE hOpen, PETH_REQUEST pPacket );
BOOL	__stdcall		ReadPacket ( HANDLE hOpen, PETH_REQUEST pPacket );
BOOL	__stdcall		SendPacketsToMstcp ( HANDLE hOpen, PETH_M_REQUEST pPackets );
BOOL	__stdcall		SendPacketsToAdapter ( HANDLE hOpen, PETH_M_REQUEST pPackets );
BOOL	__stdcall		ReadPackets ( HANDLE hOpen, PETH_M_REQUEST pPackets );
BOOL	__stdcall		SetAdapterMode ( HANDLE hOpen, PADAPTER_MODE pMode );
BOOL	__stdcall		GetAdapterMode ( HANDLE hOpen, PADAPTER_MODE pMode );
BOOL	__stdcall		FlushAdapterPacketQueue ( HANDLE hOpen, HANDLE hAdapter );
BOOL	__stdcall 		GetAdapterPacketQueueSize ( HANDLE hOpen, HANDLE hAdapter , PDWORD pdwSize);
BOOL	__stdcall		SetPacketEvent ( HANDLE hOpen, HANDLE hAdapter, HANDLE hWin32Event );
BOOL	__stdcall		SetWANEvent ( HANDLE hOpen, HANDLE hWin32Event );
BOOL	__stdcall		SetAdapterListChangeEvent ( HANDLE hOpen, HANDLE hWin32Event );
BOOL	__stdcall		NdisrdRequest ( HANDLE hOpen, PPACKET_OID_DATA OidData, BOOL Set );
BOOL	__stdcall		GetRasLinks ( HANDLE hOpen, HANDLE hAdapter, PRAS_LINKS pLinks);
BOOL	__stdcall		SetHwPacketFilter ( HANDLE hOpen, HANDLE hAdapter, DWORD Filter );
BOOL	__stdcall		GetHwPacketFilter ( HANDLE hOpen, HANDLE hAdapter, PDWORD pFilter );
BOOL	__stdcall		SetPacketFilterTable ( HANDLE hOpen, PSTATIC_FILTER_TABLE pFilterList );
BOOL	__stdcall		ResetPacketFilterTable ( HANDLE hOpen );
BOOL	__stdcall		GetPacketFilterTableSize ( HANDLE hOpen, PDWORD pdwTableSize );
BOOL	__stdcall		GetPacketFilterTable ( HANDLE hOpen, PSTATIC_FILTER_TABLE pFilterList );
BOOL	__stdcall		GetPacketFilterTableResetStats ( HANDLE hOpen, PSTATIC_FILTER_TABLE pFilterList );
BOOL	__stdcall		SetMTUDecrement ( DWORD dwMTUDecrement );
DWORD	__stdcall		GetMTUDecrement ();
BOOL	__stdcall		SetAdaptersStartupMode ( DWORD dwStartupMode );
DWORD	__stdcall		GetAdaptersStartupMode ();
BOOL	__stdcall		IsDriverLoaded ( HANDLE hOpen );
DWORD	__stdcall		GetBytesReturned ( HANDLE hOpen );

// Helper routines
BOOL __stdcall 
		ConvertWindowsNTAdapterName (
			LPCSTR szAdapterName,
			LPSTR szUserFriendlyName,
			DWORD dwUserFriendlyNameLength
			);

BOOL __stdcall 
		ConvertWindows2000AdapterName (
			LPCSTR szAdapterName,
			LPSTR szUserFriendlyName,
			DWORD dwUserFriendlyNameLength
			);

BOOL __stdcall 
		ConvertWindows9xAdapterName (
			LPCSTR szAdapterName,
			LPSTR szUserFriendlyName,
			DWORD dwUserFriendlyNameLength
			);
}