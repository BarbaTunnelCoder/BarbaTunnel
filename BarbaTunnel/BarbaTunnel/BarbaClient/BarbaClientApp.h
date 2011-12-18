#pragma once
#include "BarbaApp.h"
#include "BarbaClientConfig.h"
#include "BarbaClientConnectionManager.h"

class BarbaClientApp : public BarbaApp
{
public:
	BarbaClientApp();
	virtual ~BarbaClientApp(){}
	virtual void Initialize();
	
	BarbaClientConfigManager ConfigManager;
	BarbaClientConnectionManager ConnectionManager;
	virtual bool ProcessPacket(PacketHelper* packet, bool send);
	std::string FakeHttpGetTemplate;
	std::string FakeHttpPostTemplate;

private:
	//return pointer to BarbaClientConfigItem if the packed should grabbed before send
	BarbaClientConfigItem* ShouldGrabPacket(PacketHelper* packet, BarbaClientConfig* config);
};

extern BarbaClientApp* theClientApp;