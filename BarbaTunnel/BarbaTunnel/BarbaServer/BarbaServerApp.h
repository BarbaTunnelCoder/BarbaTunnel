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

	BarbaServerHttpHost HttpServer;
	BarbaServerConfig Config;
	BarbaServerConnectionManager ConnectionManager;
	void Initialize();
	void ProcessPacket(INTERMEDIATE_BUFFER* packet);

private:
	BarbaServerConfigItem* IsGrabPacket(PacketHelper* packet);
	void InitHttpServer();
};

extern BarbaServerApp* theServerApp;