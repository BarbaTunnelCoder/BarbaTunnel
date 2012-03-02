#pragma once
#include "BarbaBuffer.h"
#include "Base64.h"
#include "StringUtils.h"
#include "PacketHelper.h"
#include "SimpleSafeList.h"
#include "SimpleEvent.h"
#include "BarbaException.h"

#define BARBA_CurrentVersion _T("5.3")
#define BARBA_MaxConfigName 100
#define BARBA_ServerMaxVirtualIps 0xFFFF
#define BARBA_ServerMaxListenSockets 50
#define BARBA_MaxLogFileSize (1 * 100000) //(100 KB)
#define BARBA_WorkingStateRefreshTime (1*60*1000) //1 Min
#define BARBA_SocketThreadStackSize (32*1000)
#define BARBA_MaxKeyName 1000
#define BARBA_ConnectionTimeout (15 * 60*1000) //15 min
#define BARBA_HttpMaxUserConnections 20
#define BARBA_HttpFakeFileMaxSize (15 * 1000000) //15 MB
#define BARBA_HttpFakePacketMaxSize 1450 //1450 bytes
#define BARBA_HttpMaxUserConnection 5
#define BARBA_HttpKeepAliveIntervalMin (10 * 1000) //10 second
#define BARBA_HttpKeepAliveInterval (60 * 1000) //60 second
#define BARBA_ConfigFolderName _T("config")

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
void BarbaLog(LPCTSTR msg, ...);
void BarbaLog2(LPCTSTR msg, ...);
void BarbaNotify(LPCTSTR msg, ...);

