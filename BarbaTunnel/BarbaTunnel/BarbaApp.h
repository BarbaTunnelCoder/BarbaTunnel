#pragma once
#include "General.h"
#include "BarbaUtils.h"
#include "PacketHelper.h"


class BarbaApp
{
public:
	BarbaApp(void);
	virtual ~BarbaApp(void);
	virtual void Init()=0;
	virtual void ProcessPacket(INTERMEDIATE_BUFFER* packet)=0;
	//@return false to terminate process
	bool CheckTerminateCommands(INTERMEDIATE_BUFFER* packetBuffer);
	ETH_REQUEST CurrentRequest;
	bool IsDebugMode;
	DWORD GetMTUDecrement() { return sizeof iphdr + sizeof tcphdr + sizeof BarbaHeader; }
	LPCTSTR GetConfigFile() {return ConfigFile;}
	LPCTSTR GetModuleFolder() {return ModuleFolder;}
	int AdapterIndex;

private:
	INTERMEDIATE_BUFFER PacketBuffer;
	TCHAR ConfigFile[MAX_PATH];
	TCHAR ModuleFolder[MAX_PATH];
};
