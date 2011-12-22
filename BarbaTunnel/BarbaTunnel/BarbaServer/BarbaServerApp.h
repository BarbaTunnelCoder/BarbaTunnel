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
	virtual void Start();
	virtual bool ProcessPacket(PacketHelper* packet, bool send);
	virtual void Dispose();

	BarbaServerHttpHost HttpHost;
	BarbaServerConfig Config;
	BarbaServerConnectionManager ConnectionManager;
	std::string FakeHttpGetReplyTemplate;
	std::string FakeHttpPostReplyTemplate;

private:
	BarbaServerConfigItem* ShouldGrabPacket(PacketHelper* packet);
};

extern BarbaServerApp* theServerApp;