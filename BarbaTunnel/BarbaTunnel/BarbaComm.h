#pragma once

class BarbaComm
{
private:
	HANDLE _CommandEventHandle;
	TCHAR _NotifyFilePath[MAX_PATH];
	TCHAR _LogFilePath[MAX_PATH];
	TCHAR _CommFilePath[MAX_PATH];
	TCHAR _WorkFolderPath[MAX_PATH];
	HANDLE LogFile;
	HANDLE NotifyFile;
	void InitEvents();

public:
	BarbaComm(void);
	~BarbaComm(void);
	LPCTSTR GetNotifyFilePath() {return _NotifyFilePath;}
	LPCTSTR GetLogFilePath() {return _LogFilePath;}
	LPCTSTR GetCommFilePath() {return _CommFilePath;}
	LPCTSTR GetWorkFolderPath() {return _WorkFolderPath;}

	void Log(LPCTSTR msg, bool notify);
	bool CreateFiles();
	bool CreateFilesWithAdminPrompt();
};

