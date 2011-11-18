#pragma once
#include "BarbaApp.h"
#include "BarbaServerConfig.h"
#include "BarbaServerConnection.h"

class BarbaServerApp : public BarbaApp
{
public:
	BarbaServerApp(void);
	~BarbaServerApp(void);

	BarbaServerConfig Config;
	BarbaServerConnectionManager ConnectionManager;
	void Init();
	void ProcessPacket(INTERMEDIATE_BUFFER* packet);
	u_short VirtualIpInc;
};

extern BarbaServerApp* theServerApp;