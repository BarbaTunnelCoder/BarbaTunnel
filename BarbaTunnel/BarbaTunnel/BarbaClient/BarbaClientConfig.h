#pragma once
#include "General.h"
#include "BarbaUtils.h"

//BarbaClientConfigItem
class BarbaClientConfigItem
{
public:
	BarbaModeEnum Mode;
	TCHAR Name[BARBA_MAX_CONFIGNAME];
	u_char GetTunnelProtocol();
	bool IsTunnelMode() { return Mode==BarbaModeTcpTunnel || Mode==BarbaModeUdpTunnel; }
	bool IsRedirectMode() { return Mode==BarbaModeTcpRedirect || Mode==BarbaModeUdpRedirect; }
	u_short TunnelPort;
	ProtocolPort GrabProtocols[BARBA_MAX_PORTITEM]; //valid when mode is UDP-Tunnel or TCP-Tunnel mode
	int GrabProtocolsCount;
	u_short RealPort; //valid when mode is UDP-Redirect or TCP-Redirect mode
	bool Enabled;
	BarbaClientConfigItem();
};

//BarbaClientConfig
class BarbaClientConfig
{
public:
	BarbaClientConfig();
	DWORD ServerIp;
	TCHAR ServerName[BARBA_MAX_CONFIGNAME];
	BYTE Key[BARBA_MAX_KEYLEN];
	int KeyCount;
	BarbaClientConfigItem Items[BARBA_MAX_CONFIGITEMS];
	int ItemsCount;
	// @return false if could not load file
	bool LoadFile(LPCTSTR file);

private:
	//@param value: Protocol:Port,Protocol:Port ; eg: TCP:80,TCP:429,TCP:*,1:0
	static void ParseGrabProtocols(BarbaClientConfigItem* item, LPCTSTR value);
};


//BarbaClientConfigManager
class BarbaClientConfigManager
{
public:
	BarbaClientConfigManager();
	BarbaClientConfig Configs[BARBA_MAX_CONFIGS];
	int ConfigsCount;
	void LoadFolder(LPCTSTR folder);
	BarbaClientConfig* FindByServerIP(DWORD ip);
};
