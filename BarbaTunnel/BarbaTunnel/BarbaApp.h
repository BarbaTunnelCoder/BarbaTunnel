#pragma once
#include "General.h"
#include "BarbaSocket.h"
#include "BarbaUtils.h"
#include "SimpleSafeList.h"
#include "BarbaComm.h"
#include "BarbaPacketFilter.h"

class BarbaApp
{
public:
	BarbaApp(void);
	virtual ~BarbaApp(void);
	virtual void Initialize();
	virtual void Start();
	virtual bool ProcessPacket(PacketHelper* packet, bool send)=0;
	virtual void Dispose();
	virtual bool IsServerMode()=0;
	bool GetFakeFile(std::vector<std::tstring>* fakeTypes, size_t fakeFileMaxSize, TCHAR* filename, std::tstring* contentType, size_t* fileSize, std::vector<BYTE>* fakeFileHeader, bool createNew);
	bool SendPacketToOutbound(PacketHelper* packet);
	bool SendPacketToInbound(PacketHelper* packet);
	//@return false to terminate process
	bool CheckTerminateCommands(PacketHelper* packet, bool send);
	bool IsDebugMode() {return _DebugMode;}
	bool IsDisposed() {return this->_IsDisposed;}
	int GetMTUDecrement() { return this->MTUDecrement; }
	bool CheckMTUDecrement(size_t outgoingPacketLength, u_short requiredMTUDecrement);
	static LPCTSTR GetConfigFolder();
	static LPCTSTR GetSettingsFile();
	static LPCTSTR GetAppFolder();
	static LPCTSTR GetModuleFolder();
	static LPCTSTR GetModuleFile();
	BarbaPacketFilter* GetPacketFilter() { return this->PacketFilter; }
	size_t GetAdapterIndex() {return _AdapterIndex;}
	bool VerboseMode;
	BarbaComm Comm;
	u_int ConnectionTimeout;
	std::vector<FakeFileHeader> FakeFileHeaders;

	//store thread for clean shutdown; the process will wait for all of this thread to complete
	void AddThread(HANDLE threadHandle);
	void UpdateSettings();
	static void CloseFinishedThreadHandle(SimpleSafeList<HANDLE>* list);
	static void CloseSocketsList(SimpleSafeList<BarbaSocket*>* list);

private:
	int MTUDecrement;
	bool _IsDisposed;
	void InitFakeFileHeaders();
	SimpleSafeList<HANDLE> Threads;
	size_t _AdapterIndex;
	bool _DebugMode;
	TCHAR _ConfigFile[MAX_PATH];
	TCHAR _ModuleFolder[MAX_PATH];
	TCHAR _ModuleFileName[MAX_PATH];
	BarbaPacketFilter* PacketFilter;
};

extern BarbaApp* theApp;