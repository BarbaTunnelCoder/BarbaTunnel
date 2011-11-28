#pragma once
#include "General.h"
#include "BarbaUtils.h"
#include "PacketHelper.h"
#include "BarbaComm.h"


class BarbaApp
{
public:
	BarbaApp(void);
	virtual ~BarbaApp(void);
	virtual void Initialize();
	virtual void ProcessPacket(INTERMEDIATE_BUFFER* packet)=0;
	virtual void Dispose();
	//@return false to terminate process
	bool CheckTerminateCommands(INTERMEDIATE_BUFFER* packetBuffer);
	bool IsDebugMode() {return _IsDebugMode;}
	DWORD GetMTUDecrement() { return sizeof iphdr + sizeof tcphdr + sizeof BarbaHeader; }
	static LPCTSTR GetConfigFile();
	static LPCTSTR GetModuleFolder();
	static LPCTSTR GetModuleFile();
	int GetAdapterIndex() {return _AdapterIndex;}
	BarbaComm Comm;
	ETH_REQUEST CurrentRequest;

private:
	int _AdapterIndex;
	bool _IsDebugMode;
	INTERMEDIATE_BUFFER _PacketBuffer;
	TCHAR _ConfigFile[MAX_PATH];
	TCHAR _ModuleFolder[MAX_PATH];
	TCHAR _ModuleFileName[MAX_PATH];
};

extern BarbaApp* theApp;