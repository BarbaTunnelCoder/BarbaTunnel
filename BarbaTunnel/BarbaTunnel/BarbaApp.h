#pragma once
#include "General.h"
#include "BarbaSocket.h"
#include "BarbaUtils.h"
#include "SimpleSafeList.h"
#include "BarbaComm.h"
#include "BarbaFilterDriver.h"

class BarbaApp
{
public:
	BarbaApp(void);
	virtual ~BarbaApp(void);
	virtual void Initialize();
	virtual void Start();
	virtual void Stop();
	virtual bool ProcessPacket(PacketHelper* packet, bool send)=0;
	virtual bool IsServerMode()=0;
	virtual LPCTSTR GetName()=0;
	bool GetFakeFile(std::vector<std::tstring>* fakeTypes, size_t fakeFileMaxSize, TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew);
	bool SendPacketToOutbound(PacketHelper* packet);
	bool SendPacketToInbound(PacketHelper* packet);
	bool IsDebugMode() {return _DebugMode;}
	bool IsDisposed() {return this->_IsDisposed;}
	int GetMTUDecrement() { return this->MTUDecrement; }
	bool CheckMTUDecrement(size_t outgoingPacketLength, u_short requiredMTUDecrement);
	static LPCTSTR GetConfigFolder();
	static LPCTSTR GetSettingsFile();
	static LPCTSTR GetAppFolder();
	static LPCTSTR GetModuleFolder();
	static LPCTSTR GetModuleFile();
	BarbaFilterDriver* GetFilterDriver() { return this->FilterDriver; }
	size_t GetAdapterIndex() {return _AdapterIndex;}
	int LogLevel;
	BarbaComm Comm;
	u_int ConnectionTimeout;
	std::vector<FakeFileHeader> FakeFileHeaders;

	//ProcessFilterDriverPacket should called by FilterDriver
	//@return false if packet does not processed and should just continue its route
	bool ProcessFilterDriverPacket(PacketHelper* packet, bool send);

	//store thread for clean shutdown; the process will wait for all of this thread to complete
	void AddThread(HANDLE threadHandle);
	void UpdateSettings();
	static void CloseFinishedThreadHandle(SimpleSafeList<HANDLE>* list);
	static void CloseSocketsList(SimpleSafeList<BarbaSocket*>* list);
	//called by BarbaComm
	virtual void OnNewCommand(BarbaComm::CommandEnum command);

protected:
	virtual void Dispose();

private:
	//@return false to terminate the process
	bool CheckTerminateCommands(PacketHelper* packet, bool send);
	unsigned int CommandMonitorThread(void* data);
	int MTUDecrement;
	bool _IsDisposed;
	void InitFakeFileHeaders();
	SimpleSafeList<HANDLE> Threads;
	size_t _AdapterIndex;
	bool _DebugMode;
	TCHAR _ConfigFile[MAX_PATH];
	TCHAR _ModuleFolder[MAX_PATH];
	TCHAR _ModuleFileName[MAX_PATH];
	BarbaFilterDriver* FilterDriver;
	bool IsRestartCommand;
};

extern BarbaApp* theApp;