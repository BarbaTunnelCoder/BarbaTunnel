#pragma once
#include "BarbaApp.h"
#include "BarbaClientConfig.h"
#include "BarbaClientConnectionManager.h"

class BarbaClientApp : public BarbaApp
{
public:
	BarbaClientApp();
	virtual ~BarbaClientApp();
	void Initialize() override;
	void Dispose() override;
	bool IsServerMode() override {return false;}
	LPCTSTR GetName() override {return _T("Barba Client");}
	BarbaClientConfig* FindConfigForOutboundPacket(PacketHelper* packet);
	BarbaClientConfig* FindConfigForInboundPacket(PacketHelper* packet);
	bool ProcessOutboundPacket(PacketHelper* packet) override;
	bool ProcessInboundPacket(PacketHelper* packet) override;
	static bool IsConfigForInboundPacket(BarbaClientConfig* config, PacketHelper* packet);
	static bool IsConfigForOutboundPacket(BarbaClientConfig* config, PacketHelper* packet);
	
	BarbaArray<BarbaClientConfig> Configs;
	BarbaClientConnectionManager ConnectionManager;
	std::string HttpGetTemplate;
	std::string HttpPostTemplate;
	std::string HttpGetTemplateBombard;
	std::string HttpPostTemplateBombard;

protected:
	void Load() override;
};

extern BarbaClientApp* theClientApp;