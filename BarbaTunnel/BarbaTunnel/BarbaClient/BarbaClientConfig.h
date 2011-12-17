#pragma once
#include "BarbaConfigItem.h"

//BarbaClientConfigItem
class BarbaClientConfigItem : public BarbaConfigItem
{
public:
	virtual ~BarbaClientConfigItem(){}
	u_short GetNewTunnelPort();
	ProtocolPort GrabProtocols[BARBA_MAX_PORTITEM]; //valid when mode is UDP-Tunnel or TCP-Tunnel mode
	size_t GrabProtocolsCount;
	BarbaClientConfigItem();
	bool ShouldGrabPacket(PacketHelper* packet);
};

//BarbaClientConfig
class BarbaClientConfig
{
public:
	BarbaClientConfig();
	DWORD ServerIp;
	TCHAR ServerName[BARBA_MAX_CONFIGNAME];
	BarbaKey Key;
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
