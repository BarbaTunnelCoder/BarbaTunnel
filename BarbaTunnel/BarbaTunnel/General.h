#pragma once

#define MAX_BARBA_CONFIGS 100
#define MAX_BARBA_CONFIGITEMS 100
#define MAX_BARBA_CONNECTIONS 1000
#define BARBA_MAX_PORTITEM 100
#define BARBA_MAX_KEYLEN 1500

//BarbaModeEnum
enum BarbaModeEnum
{
	BarbaModeNone,
	BarbaModeUdpTunnel,
	BarbaModeUdpRedirect,
	BarbaModeTcpTunnel,
	BarbaModeTcpRedirect,
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


BarbaModeEnum BarbaMode_FromString(TCHAR* mode);
u_char BarbaMode_GetProtocol(BarbaModeEnum mode);
GUID* GetBarbaSign();
