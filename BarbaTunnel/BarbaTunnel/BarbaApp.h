#pragma once
#include "General.h"
#include "BarbaUtils.h"
#include "PacketHelper.h"
#include "BarbaHeader.h"
#include "BarbaConfig.h"
#include "BarbaConnection.h"

class BarbaApp
{
public:
	BarbaApp();
	void Init();
	bool IsServer();
	BarbaConfigManager ConfigManager;
	BarbaConnectionManager ConnectionManager;
	GUID* GetBarbaSign();
	void ProcessPacket(INTERMEDIATE_BUFFER* packet);
	//@return false to terminate process
	bool CheckTerminateCommands(INTERMEDIATE_BUFFER* packet);
	ETH_REQUEST CurrentRequest;
	bool IsDebugMode;
	u_short IpInc;

private:
	bool IsClientGrabPacket(PacketHelper* packet, BarbaConfig* config);
	INTERMEDIATE_BUFFER PacketBuffer;

};

extern BarbaApp* theApp;