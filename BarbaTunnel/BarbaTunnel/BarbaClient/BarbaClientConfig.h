#pragma once
#include "BarbaConfigItem.h"

//BarbaClientConfigItem
class BarbaClientConfigItem : public BarbaConfigItem
{
public:
	virtual ~BarbaClientConfigItem(){}
	u_short GetNewTunnelPort();
	std::vector<ProtocolPort> GrabProtocols;//valid when in Tunnel mode
	BarbaClientConfigItem();
	bool ShouldGrabPacket(PacketHelper* packet);
};

//BarbaClientConfig
class BarbaClientConfig
{
public:
	BarbaClientConfig();
	DWORD ServerIp;
	TCHAR ServerName[BARBA_MaxConfigName];
	std::vector<BarbaClientConfigItem> Items;
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
	std::vector<BarbaClientConfig> Configs;
	void LoadFolder(LPCTSTR folder);
	BarbaClientConfig* FindByServerIP(DWORD ip);
};
