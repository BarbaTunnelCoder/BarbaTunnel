#pragma once
#include "BarbaApp.h"
#include "BarbaServerConfig.h"
#include "BarbaServerConnectionManager.h"
#include "BarbaServerHttpHost.h"

class BarbaServerApp : public BarbaApp
{
public:
	BarbaServerApp(void);
	virtual ~BarbaServerApp(void);
	virtual void Initialize();
	virtual bool ProcessPacket(PacketHelper* packet, bool send);

	BarbaServerHttpHost HttpServer;
	BarbaServerConfig Config;
	BarbaServerConnectionManager ConnectionManager;

private:
	BarbaServerConfigItem* ShouldGrabPacket(PacketHelper* packet);
};

extern BarbaServerApp* theServerApp;