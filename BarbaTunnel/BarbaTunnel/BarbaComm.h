#pragma once
#include "SimpleEvent.h"

class BarbaComm
{
public:
	enum CommandEnum{
		CommandNone,
		CommandStop,
		CommandRestart,
		CommandUpdateSettings,
	};

private:
	TCHAR _NotifyFilePath[MAX_PATH];
	TCHAR _LogFilePath[MAX_PATH];
	TCHAR _CommFilePath[MAX_PATH];
	TCHAR _WorkFolderPath[MAX_PATH];
	HANDLE LogFileHandle;
	HANDLE NotifyFileHandle;
	void InitializeEvents();
	bool _IsAlreadyRunning;
	DWORD LastWorkingTick;
	SimpleEvent DisposeEvent;
	SimpleEvent CommandEvent;
	static unsigned int __stdcall CommandMonitorThread(void* data);
	HANDLE CommandMonitorThreadHandle;

public:
	BarbaComm(void);
	virtual ~BarbaComm(void);
	virtual void Dispose();
	bool IsDisposing() { return this->DisposeEvent.IsSet(); }
	void Initialize();
	LPCTSTR GetNotifyFilePath() {return _NotifyFilePath;}
	LPCTSTR GetLogFilePath() {return _LogFilePath;}
	LPCTSTR GetCommFilePath() {return _CommFilePath;}
	LPCTSTR GetWorkFolderPath() {return _WorkFolderPath;}
	CommandEnum GetCommand();
	void SetWorkingState(size_t length, bool send);
	void SetStatus(LPCTSTR status);
	void Log(LPCTSTR msg, bool notify);
	bool CreateFiles();
	bool IsAlreadyRunning() {return _IsAlreadyRunning;}
	HANDLE GetNotifyFileHandle() { return this->NotifyFileHandle; }
};

