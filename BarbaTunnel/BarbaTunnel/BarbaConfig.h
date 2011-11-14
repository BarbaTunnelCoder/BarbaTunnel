#pragma once
#include "General.h"
#include "BarbaUtils.h"

class BarbaConfig
{
public:
	DWORD ServerIP;
	ProtocolPort TunnelProtocol;
	ProtocolPort GrabProtocols[100];
	int GrabProtocolsCount;
	BYTE Key[255];

	BarbaConfig();
	void LoadFile(LPCTSTR file);

private:
	//@param value: Protocol:Port,Protocol:Port ; eg: TCP:80,TCP:429,TCP:*,1:0
	void ParseGrabProtocols(LPCTSTR value);
};

class BarbaConfigManager
{
private:
	BarbaConfig* Configs[MAX_BARBA_CONFIGS];
	int ConfigsCount;
	BarbaConfig* ServerConfig;

public:
	BarbaConfigManager();
	BarbaConfig* GetServerConfig();
	~BarbaConfigManager();
	void LoadFolder(LPCTSTR folder);
	BarbaConfig* FindByServerIP(DWORD ip);
};
