#pragma once
#include "Base64.h"
#include "StringUtils.h"
#include "PacketHelper.h"
#include "SimpleSafeList.h"
#include "BarbaException.h"

#define BARBA_MAX_VIRTUALIP 0xFFFF
#define BARBA_MaxConfigName 100
#define BARBA_MAX_SERVERLISTENSOCKET 50
#define BARBA_MaxLogFileSize (1000) //1000K (1MB)
#define BARBA_CURRENT_VERSION _T("2.1")
#define BARBA_WorkingStateRefreshTime (1*60*1000)
#define BARBA_SocketThreadStackSize (32*1000)
#define BARBA_MaxKeyName 1000
#define BARBA_ConnectionTimeout 15 //15 min
#define BARBA_HttpMaxUserConnections 20
#define BARBA_HttpMaxFakeFileSize 15000 //15 MB
#define BARBA_HttpMaxUserConnection 5

//BarbaModeEnum
enum BarbaModeEnum
{
	BarbaModeNone,
	BarbaModeUdpTunnel,
	BarbaModeUdpRedirect,
	BarbaModeTcpTunnel,
	BarbaModeTcpRedirect,
	BarbaModeHttpTunnel,
};

//PortRange
struct PortRange
{
	u_short StartPort;
	u_short EndPort;
};

//IpRange
struct IpRange
{
	DWORD StartIp;
	DWORD EndIp;
};


//ProtocolPort
struct ProtocolPort
{
	BYTE Protocol;
	u_short Port; 
};

//FakeFileHeader
struct FakeFileHeader
{
	std::tstring ContentType;
	std::tstring Extension;
	std::vector<BYTE> Data;
};

//BarbaHeader. Not used yet
struct BarbaHeader
{
	GUID Signature;
	BYTE Version;	
};

BarbaModeEnum BarbaMode_FromString(LPCTSTR mode);
LPCTSTR BarbaMode_ToString(BarbaModeEnum mode);
u_char BarbaMode_GetProtocol(BarbaModeEnum mode);
void BarbaLog(LPCTSTR msg, ...);
void BarbaLog2(LPCTSTR msg, ...);
void BarbaNotify(LPCTSTR msg, ...);

