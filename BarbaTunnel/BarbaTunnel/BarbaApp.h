#pragma once
#include "General.h"
#include "BarbaSocket.h"
#include "BarbaUtils.h"
#include "BarbaComm.h"
#include "SimpleSafeList.h"

class BarbaApp
{
public:
	BarbaApp(void);
	virtual ~BarbaApp(void);
	virtual void Initialize();
	virtual void Start();
	virtual bool ProcessPacket(PacketHelper* packet, bool send)=0;
	virtual void Dispose();
	bool SendPacketToAdapter(PacketHelper* packet);
	bool SendPacketToMstcp(PacketHelper* packet);
	//@return false to terminate process
	bool CheckTerminateCommands(PacketHelper* packet, bool send);
	bool IsDebugMode() {return _DebugMode;}
	DWORD GetMTUDecrement() { return sizeof iphdr + sizeof tcphdr + sizeof BarbaHeader; }
	static LPCTSTR GetConfigFile();
	static LPCTSTR GetModuleFolder();
	static LPCTSTR GetModuleFile();
	int GetAdapterIndex() {return _AdapterIndex;}
	BarbaComm Comm;
	HANDLE hAdapterHandle;
	u_int ConnectionTimeout;

	//store thread for clean shutdown; the process will wait for all of this thread to complete
	void AddThread(HANDLE threadHandle);
	static void CloseFinishedThreadHandle(SimpleSafeList<HANDLE>* list);
	static void CloseSocketsList(SimpleSafeList<BarbaSocket*>* list);


private:
	SimpleSafeList<HANDLE> Threads;
	int _AdapterIndex;
	bool _VerboseMode;
	bool _DebugMode;
	TCHAR _ConfigFile[MAX_PATH];
	TCHAR _ModuleFolder[MAX_PATH];
	TCHAR _ModuleFileName[MAX_PATH];
};

extern BarbaApp* theApp;