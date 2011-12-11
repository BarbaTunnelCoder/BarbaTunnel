#pragma once
#include "BarbaApp.h"
#include "BarbaClientConfig.h"
#include "BarbaClientConnection.h"

class BarbaClientApp : public BarbaApp
{
public:
	BarbaClientApp();
	virtual ~BarbaClientApp(){}
	void Initialize();
	BarbaClientConfigManager ConfigManager;
	BarbaClientConnectionManager ConnectionManager;
	GUID* GetBarbaSign();
	void ProcessPacket(INTERMEDIATE_BUFFER* packet);

private:
	//return pointer to BarbaClientConfigItem if the packed should grabbed before send
	BarbaClientConfigItem* IsGrabPacket(PacketHelper* packet, BarbaClientConfig* config);
	bool IsGrabPacket(PacketHelper* packet, BarbaClientConfigItem* configItem);

};

extern BarbaClientApp* theClientApp;