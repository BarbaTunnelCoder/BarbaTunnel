#pragma once
#include "BarbaConfig.h"
#include "BarbaCourierRequestMode.h"

//BarbaClientConfig
class BarbaClientConfig : public BarbaConfig
{
public:
	BarbaClientConfig();
	virtual bool LoadFile(LPCTSTR file);
	virtual ~BarbaClientConfig(){}
	static void LoadFolder(LPCTSTR folder, std::vector<BarbaClientConfig>* configs);
	u_short MinPacketSize;
	DWORD KeepAliveInterval;
	std::vector<ProtocolPort> GrabProtocols;
	BarbaCourierRequestMode HttpRequestMode;
};
