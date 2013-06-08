#pragma once
#include "BarbaApp.h"
#include "BarbaServerConfig.h"
#include "BarbaServerConnectionManager.h"
#include "BarbaServerTcpHost.h"

class BarbaServerApp : public BarbaApp
{
public:
	explicit BarbaServerApp(bool delayStart);
	virtual ~BarbaServerApp(void);
	void Initialize() override;
	void Start() override;
	void Dispose() override;
	bool IsServerMode() override {return true;}
	LPCTSTR GetName() override {return _T("Barba Server");}
	BarbaServerConfig* FindConfigForOutboundPacket(PacketHelper* packet);
	BarbaServerConfig* FindConfigForInboundPacket(PacketHelper* packet);
	bool ProcessOutboundPacket(PacketHelper* packet) override;
	bool ProcessInboundPacket(PacketHelper* packet) override;
	static bool IsConfigForInboundPacket(BarbaServerConfig* config, PacketHelper* packet);

	BarbaServerTcpHost HttpHost;
	BarbaArray<BarbaServerConfig> Configs;
	BarbaServerConnectionManager ConnectionManager;
	std::tstring HttpGetReplyTemplate;
	std::tstring HttpGetReplyTemplateBombard;
	std::tstring HttpPostReplyTemplate;
	std::tstring HttpPostReplyTemplateBombard;
	u_int AutoStartDelay;
	IpRange VirtualIpRange;

protected:
	void Load() override;

private:
	bool DelayStart;
};

extern BarbaServerApp* theServerApp;