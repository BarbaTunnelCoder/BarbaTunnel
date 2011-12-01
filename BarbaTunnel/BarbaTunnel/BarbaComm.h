#pragma once

class BarbaComm
{
public:
	enum CommandEnum{
		CommandNone,
		CommandStop,
		CommandRestart,
	};

private:
	HANDLE _CommandEventHandle;
	TCHAR _NotifyFilePath[MAX_PATH];
	TCHAR _LogFilePath[MAX_PATH];
	TCHAR _CommFilePath[MAX_PATH];
	TCHAR _WorkFolderPath[MAX_PATH];
	HANDLE LogFileHandle;
	HANDLE NotifyFileHandle;
	void InitializeEvents();
	bool _IsAlreadyRunning;
	DWORD _LastWorkingTick;

public:
	BarbaComm(void);
	~BarbaComm(void);
	void Initialize();
	LPCTSTR GetNotifyFilePath() {return _NotifyFilePath;}
	LPCTSTR GetLogFilePath() {return _LogFilePath;}
	LPCTSTR GetCommFilePath() {return _CommFilePath;}
	LPCTSTR GetWorkFolderPath() {return _WorkFolderPath;}
	CommandEnum GetCommand();
	void SetWorkingState(ULONG length, bool send);
	void SetStatus(LPCTSTR status);
	void Log(LPCTSTR msg, bool notify);
	bool CreateFiles();
	bool CreateFilesWithAdminPrompt();
	bool IsAlreadyRunning() {return _IsAlreadyRunning;}
	void Dispose();
};

