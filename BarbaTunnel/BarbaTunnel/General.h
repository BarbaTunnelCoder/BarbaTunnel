#pragma once
#include "PacketHelper.h"
#include "SimpleBuffer.h"
#include "BarbaException.h"

#define BARBA_MAX_CONFIGS 100
#define BARBA_MAX_CONFIGITEMS 100
#define BARBA_MAX_CONNECTIONS 1000
#define BARBA_MAX_PORTITEM 100
#define BARBA_MAX_KEYLEN 1500
#define BARBA_MAX_VIRTUALIP 0xFFFF
#define BARBA_MAX_CONFIGNAME 100
#define BARBA_MAX_SERVERLISTENSOCKET 50
#define BARBA_MAX_LOGFILESIZE (1 * 1000000) //1MB
#define BARBA_CURRENT_VERSION _T("2.0")
#define BARBA_WorkingStateRefreshTime (1*60*1000)
#define BARBA_SocketThreadStackSize (32*1000)
#define BARBA_MaxUserHttpConnection 20
#define BARBA_MaxKeyName 100
#define BARBA_ConnectionTimeoutMinutes (15) //15 min
#define BARBA_HttpFakeFileSize (15*1000000) //15 MB
#define BARBA_HttpMaxUserConnection 4

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


//BarbaHeader
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

