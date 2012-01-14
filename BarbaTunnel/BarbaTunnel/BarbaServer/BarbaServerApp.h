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
	virtual bool IsServerMode() {return true;}
	static bool ShouldGrabPacket(PacketHelper* packet, BarbaServerConfig* config);

	BarbaServerHttpHost HttpHost;
	std::vector<BarbaServerConfig> Configs;
	BarbaServerConnectionManager ConnectionManager;
	std::tstring FakeHttpGetReplyTemplate;
	std::tstring FakeHttpPostReplyTemplate;
	u_int AutoStartDelay;
	IpRange VirtualIpRange;

private:
	BarbaServerConfig* ShouldGrabPacket(PacketHelper* packet);
};

extern BarbaServerApp* theServerApp;