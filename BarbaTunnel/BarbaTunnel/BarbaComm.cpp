#include "StdAfx.h"
#include "General.h"
#include "BarbaComm.h"
#include "BarbaUtils.h"


BarbaComm::BarbaComm(void)
{
	TCHAR programData[MAX_PATH];
	BOOL res = SHGetSpecialFolderPath(NULL, programData, CSIDL_COMMON_APPDATA, TRUE);
	if (res)
	{
		_stprintf_s(_WorkFolderPath, _T("%s\\Barbatunnel"), programData);
		_stprintf_s(_CommFilePath, _T("%s\\comm.txt"), _WorkFolderPath);
		_stprintf_s(_LogFilePath, _T("%s\\report.txt"), _WorkFolderPath);
		_stprintf_s(_NotifyFilePath, _T("%s\\notify.txt"), _WorkFolderPath);
	}

	//events
	InitEvents();
}


BarbaComm::~BarbaComm(void)
{
	CloseHandle(LogFile);
	CloseHandle(NotifyFile);
}

void BarbaComm::InitEvents()
{
	//let all user see events
	SECURITY_DESCRIPTOR sd = { 0 };
	::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	::SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;

	_CommandEventHandle = CreateEvent(&sa, TRUE, FALSE, _T("Global\\BarbaTunnel_CommandEvent"));
	if (_CommandEventHandle==NULL) BarbaLog(_T("Could not create Global\\BarbaTunnel_CommandEvent!\n"));
}

void BarbaComm::Log(LPCTSTR msg, bool notify)
{
	LPCSTR msgUtf8;
	//write UTF8
#ifdef _UNICODE
	CHAR bufA[5000];
	WideCharToMultiByte(CP_UTF8, 0, decorateMessage, -1, bufA, _countof(bufA), 0, 0);
	msgUtf8 = bufA;
#else 
	msgUtf8 = msg;
#endif
	DWORD writeLen;

	if (notify)
	{
		SetFilePointer(NotifyFile, 0, 0, FILE_BEGIN);
		WriteFile(NotifyFile, msgUtf8, strlen(msgUtf8), &writeLen, NULL);
		SetEndOfFile(NotifyFile);
	}
	else
	{
		_tprintf_s(msg);
		_tprintf_s(_T("\r\n"));
		WriteFile(LogFile, msgUtf8, strlen(msgUtf8), &writeLen, NULL);
		WriteFile(LogFile, _T("\r\n"), 2, &writeLen, NULL);
		SetEndOfFile(LogFile);
	}
}

bool BarbaComm::CreateFilesWithAdminPrompt()
{
	CreateDirectory(GetWorkFolderPath(), NULL);
	TCHAR cmd[MAX_PATH*2];
	_stprintf_s(cmd, _T("\"%s\" /e /g everyone:c /t"), GetWorkFolderPath());
	BarbaUtils::SimpleShellExecute(_T("cacls.exe"), cmd, NULL, SW_HIDE, _T("runas"));
	return CreateFiles();
}

bool BarbaComm::CreateFiles()
{
	CreateDirectory(GetWorkFolderPath(), NULL);
	LogFile = CreateFile(GetLogFilePath(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	NotifyFile = CreateFile(GetNotifyFilePath(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE handle2 = CreateFile(GetCommFilePath(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle2!=INVALID_HANDLE_VALUE)
		CloseHandle(handle2);
	
	return LogFile!=INVALID_HANDLE_VALUE &&  handle2!=INVALID_HANDLE_VALUE;
}
