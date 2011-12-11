#pragma once
#include "BarbaApp.h"
#include "BarbaVirtualIp.h"
#include "BarbaServerConfig.h"
#include "BarbaServerConnection.h"

class BarbaServerApp : public BarbaApp
{
public:
	BarbaServerApp(void);
	virtual ~BarbaServerApp(void);

	BarbaServerConfig Config;
	BarbaServerConnectionManager ConnectionManager;
	void Initialize();
	void ProcessPacket(INTERMEDIATE_BUFFER* packet);
	BarbaVirtualIpManager VirtualIpManager;

private:
	BarbaServerConfigItem* IsGrabPacket(PacketHelper* packet);
};

extern BarbaServerApp* theServerApp;