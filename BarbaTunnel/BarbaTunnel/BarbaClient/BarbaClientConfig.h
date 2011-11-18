#pragma once
#include "General.h"
#include "BarbaUtils.h"

//BarbaClientConfigItem
class BarbaClientConfigItem
{
public:
	BarbaModeEnum Mode;
	u_char GetTunnelProtocol();
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
	BYTE Key[BARBA_MAX_KEYLEN];
	int KeyCount;
	BarbaClientConfigItem Items[MAX_BARBA_CONFIGITEMS];
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
	BarbaClientConfig Configs[MAX_BARBA_CONFIGS];
	int ConfigsCount;
	void LoadFolder(LPCTSTR folder);
	BarbaClientConfig* FindByServerIP(DWORD ip);
};
