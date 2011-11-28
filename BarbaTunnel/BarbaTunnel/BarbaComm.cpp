#include "StdAfx.h"
#include "General.h"
#include "BarbaApp.h"


BarbaComm::BarbaComm(void)
{
	_IsAlreadyRunning = false;
	LogFileHandle = NULL;
	NotifyFileHandle = NULL;
	_CommandEventHandle = NULL;
	TCHAR programData[MAX_PATH];
	BOOL res = SHGetSpecialFolderPath(NULL, programData, CSIDL_COMMON_APPDATA, TRUE);
	if (res)
	{
		_stprintf_s(_WorkFolderPath, _T("%s\\Barbatunnel"), programData);
		_stprintf_s(_CommFilePath, _T("%s\\comm.txt"), _WorkFolderPath);
		_stprintf_s(_LogFilePath, _T("%s\\report.txt"), _WorkFolderPath);
		_stprintf_s(_NotifyFilePath, _T("%s\\notify.txt"), _WorkFolderPath);
	}
}

void BarbaComm::Initialize()
{
	InitializeEvents();
}


BarbaComm::~BarbaComm(void)
{
	Dispose();
}

void BarbaComm::Dispose()
{
	CloseHandle(LogFileHandle); LogFileHandle = NULL;
	CloseHandle(NotifyFileHandle); NotifyFileHandle = NULL;
	CloseHandle(_CommandEventHandle); _CommandEventHandle = NULL;
}

void BarbaComm::InitializeEvents()
{
	//let all user see events
	SECURITY_DESCRIPTOR sd = { 0 };
	::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	::SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;

	_CommandEventHandle = CreateEvent(&sa, TRUE, FALSE, _T("Global\\BarbaTunnel_CommandEvent"));
	_IsAlreadyRunning = GetLastError()==ERROR_ALREADY_EXISTS;
	if (_CommandEventHandle==NULL) BarbaLog(_T("Could not create Global\\BarbaTunnel_CommandEvent!\n"));
}

void BarbaComm::SetStatus(LPCTSTR status)
{
	//Status
	WritePrivateProfileString(_T("General"), _T("Status"), status, GetCommFilePath());

	//write StatusTime
	TCHAR ctimeBuf[200];
	time_t rawtime;
	time ( &rawtime );
	_tctime_s(ctimeBuf, _countof(ctimeBuf), &rawtime);
	WritePrivateProfileString(_T("General"), _T("StatusTime"), ctimeBuf, GetCommFilePath());
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
		SetFilePointer(NotifyFileHandle, 0, 0, FILE_BEGIN);
		WriteFile(NotifyFileHandle, msgUtf8, strlen(msgUtf8), &writeLen, NULL);
		SetEndOfFile(NotifyFileHandle);
	}
	else
	{
		_tprintf_s(msg);
		_tprintf_s(_T("\r\n"));
		WriteFile(LogFileHandle, msgUtf8, strlen(msgUtf8), &writeLen, NULL);
		WriteFile(LogFileHandle, _T("\r\n"), 2, &writeLen, NULL);
		SetEndOfFile(LogFileHandle);
	}
}

bool BarbaComm::CreateFilesWithAdminPrompt()
{
	CreateDirectory(GetWorkFolderPath(), NULL);
	TCHAR cmd[MAX_PATH*2];
	_stprintf_s(cmd, _T("\"%s\" /e /g everyone:c /t"), GetWorkFolderPath());
	BarbaUtils::SimpleShellExecuteAndWait(_T("cacls.exe"), cmd, NULL, SW_HIDE, _T("runas"));
	return CreateFiles();
}

bool BarbaComm::CreateFiles()
{
	CreateDirectory(GetWorkFolderPath(), NULL);
	LogFileHandle = CreateFile(GetLogFilePath(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	NotifyFileHandle = CreateFile(GetNotifyFilePath(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE commFileHandle = CreateFile(GetCommFilePath(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (commFileHandle!=INVALID_HANDLE_VALUE)
		CloseHandle(commFileHandle);
	
	return LogFileHandle!=INVALID_HANDLE_VALUE && NotifyFileHandle!=INVALID_HANDLE_VALUE && commFileHandle!=INVALID_HANDLE_VALUE;
}

BarbaComm::CommandEnum BarbaComm::GetCommand()
{
	TCHAR buf[200];
	GetPrivateProfileString(_T("General"), _T("Command"), _T(""), buf, _countof(buf), GetCommFilePath());
	if (_tcsicmp(_T("Restart"), buf)==0) return CommandRestart;
	if (_tcsicmp(_T("Stop"), buf)==0) return CommandStop;
	return CommandNone;
}

void BarbaLog(LPCTSTR format, ...)
{
	va_list argp;
	va_start(argp, format);
	CHAR msg[10000];
	_vstprintf_s(msg, format, argp);
	va_end(argp);

	if (theApp!=NULL)
	{
		theApp->Comm.Log(msg, false);
	}
	else
	{
		_tprintf_s(msg);
		_tprintf_s(_T("\r\n"));
	}
}

void BarbaNotify(LPCTSTR format, ...)
{
	va_list argp;
	va_start(argp, format);
	CHAR msg[10000];
	_vstprintf_s(msg, format, argp);
	va_end(argp);

	if (theApp!=NULL)
	{
		theApp->Comm.Log(msg, true);
	}
	else
	{
		_tprintf_s(msg);
		_tprintf_s(_T("\r\n"));
	}
}
