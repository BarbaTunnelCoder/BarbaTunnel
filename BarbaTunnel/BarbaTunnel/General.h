#pragma once
#include "BarbaBuffer.h"
#include "Base64.h"
#include "StringUtils.h"
#include "PacketHelper.h"
#include "SimpleSafeList.h"
#include "SimpleEvent.h"
#include "BarbaException.h"

#define BARBA_CurrentVersion _T("8.0")
#define BARBA_MaxConfigName 100
#define BARBA_ServerMaxVirtualIps 0xFFFF
#define BARBA_ServerMaxListenSockets 200
#define BARBA_MaxLogFileSize (1 * 100000) //(100 KB)
#define BARBA_WorkingStateRefreshTime (1*60*1000) //1 Min
#define BARBA_SocketThreadStackSize (8*1000)
#define BARBA_MaxKeyName 1000
#define BARBA_ConnectionTimeout (15 * 60*1000) //15 min
#define BARBA_MaxUserConnections 100
#define BARBA_MaxUserConnectionsDefault 5
#define BARBA_MaxTransferSize (15 * 1000000) //15 MB
#define BARBA_MinPacketSizeLimit 1450 //1450 bytes
#define BARBA_KeepAliveIntervalMin (10 * 1000) //10 second
#define BARBA_KeepAliveIntervalDefault (60 * 1000) //60 second
#define BARBA_ConfigFolderName _T("config")
#define BARBA_ConnectionTimeout (15 * 60*1000) //15 min


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
	BarbaBuffer Data;
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
void BarbaLogImpl(int level, LPCTSTR msg, va_list _ArgList);
void BarbaLog(LPCTSTR msg, ...);
void BarbaLog1(LPCTSTR msg, ...);
void BarbaLog2(LPCTSTR msg, ...);
void BarbaLog3(LPCTSTR msg, ...);
void BarbaNotify(LPCTSTR msg, ...);

