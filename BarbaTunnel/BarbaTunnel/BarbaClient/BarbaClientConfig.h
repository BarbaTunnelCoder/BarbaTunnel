#pragma once
#include "BarbaConfig.h"

//BarbaClientConfig
class BarbaClientConfig : public BarbaConfig
{
public:
	BarbaClientConfig();
	virtual bool LoadFile(LPCTSTR file);
	virtual ~BarbaClientConfig(){}
	static void LoadFolder(LPCTSTR folder, std::vector<BarbaClientConfig>* configs);
	u_short GetNewTunnelPort();
	u_short FakePacketMinSize;
	DWORD KeepAliveInterval;
	std::vector<ProtocolPort> GrabProtocols; //valid when in Tunnel mode
	std::tstring RequestBombard;
};
