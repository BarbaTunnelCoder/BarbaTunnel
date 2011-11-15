#pragma once
#include "General.h"
#include "BarbaUtils.h"

//BarbaConfig
class ClientConfigItem
{
public:
	BarbaModeEnum Mode;
	u_short TunnelPort;
	ProtocolPort GrabProtocols[MAX_PORTITEM]; //valid when mode is UDP-Tunnel or TCP-Tunnel mode
	int GrabProtocolsCount;
	u_short RealPort; //valid when mode is UDP-Redirect or TCP-Redirect mode
	bool Enabled;


	ClientConfigItem();
	void LoadFile(LPCTSTR file);

private:
	//@param value: Protocol:Port,Protocol:Port ; eg: TCP:80,TCP:429,TCP:*,1:0
	void ParseGrabProtocols(LPCTSTR value);
};

//BarbaConfig
class ClientConfig
{
public:
	DWORD ServerIp;
	BYTE Key[MAX_KEYLEN];
	ClientConfigItem Items[MAX_BARBA_CONFIGITEMS];
};


//BarbaConfigManager
class ClientConfigManager
{
public:
	ClientConfigManager();
	ClientConfig* Configs[MAX_BARBA_CONFIGS];
	int ConfigsCount;
	void LoadFolder(LPCTSTR folder);
	ClientConfig* FindByServerIP(DWORD ip);
};
