#pragma once
#include "BarbaApp.h"
#include "BarbaServerConfig.h"
#include "BarbaServerConnectionManager.h"
#include "BarbaServerHttpHost.h"

class BarbaServerApp : public BarbaApp
{
public:
	explicit BarbaServerApp(bool delayStart);
	virtual ~BarbaServerApp(void);
	virtual void Initialize();
	virtual void Start();
	virtual bool ProcessPacket(PacketHelper* packet, bool send);
	virtual void Dispose();
	virtual bool IsServerMode() {return true;}
	virtual LPCTSTR GetName() {return _T("Barba Server");}
	static bool ShouldGrabPacket(PacketHelper* packet, BarbaServerConfig* config);

	BarbaServerHttpHost HttpHost;
	std::vector<BarbaServerConfig> Configs;
	BarbaServerConnectionManager ConnectionManager;
	std::tstring HttpGetReplyTemplate;
	std::tstring HttpGetReplyTemplateBombard;
	std::tstring HttpPostReplyTemplate;
	std::tstring HttpPostReplyTemplateBombard;
	u_int AutoStartDelay;
	IpRange VirtualIpRange;

private:
	BarbaServerConfig* ShouldGrabPacket(PacketHelper* packet);
	bool DelayStart;
};

extern BarbaServerApp* theServerApp;