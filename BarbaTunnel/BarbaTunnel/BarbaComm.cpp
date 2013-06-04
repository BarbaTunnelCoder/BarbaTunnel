#include "StdAfx.h"
#include "General.h"
#include "BarbaApp.h"

BarbaComm* theComm = NULL;
BarbaComm::BarbaComm(void)
	: DisposeEvent(true, false)
{
	theComm = this;
	this->_IsAlreadyRunning = false;
	this->LogFileHandle = NULL;
	this->NotifyFileHandle = NULL;
	this->LastWorkingTick = 0;
	TCHAR programData[MAX_PATH];
	BOOL res = SHGetSpecialFolderPath(NULL, programData, CSIDL_COMMON_APPDATA, TRUE);
	if (res)
	{
		_stprintf_s(_WorkFolderPath, _T("%s\\BarbaTunnel"), programData);
		_stprintf_s(_CommFilePath, _T("%s\\comm.txt"), _WorkFolderPath);
		_stprintf_s(_LogFilePath, _T("%s\\report.txt"), _WorkFolderPath);
		_stprintf_s(_NotifyFilePath, _T("%s\\notify.txt"), _WorkFolderPath);
	}
}

void BarbaComm::Initialize()
{
	InitializeEvents();
	if (IsAlreadyRunning())
		throw new BarbaException("BarbaTunnel already running!");

	if (!CreateFiles())
		throw new BarbaException("Could not prepare BarbaComm files!");
}


BarbaComm::~BarbaComm(void)
{
}

void BarbaComm::Dispose()
{
	DisposeEvent.Set();
	WaitForSingleObject(this->CommandMonitorThreadHandle, INFINITE);
	CloseHandle(this->CommandMonitorThreadHandle); this->CommandMonitorThreadHandle = NULL;
	CloseHandle(this->LogFileHandle); this->LogFileHandle = NULL;
	CloseHandle(this->NotifyFileHandle); this->NotifyFileHandle = NULL;
	CommandEvent.Close();
}

unsigned int BarbaComm::CommandMonitorThread(void* data)
{
	BarbaComm* _this = (BarbaComm*)data;
	HANDLE events[2];
	events[0] = _this->CommandEvent.GetHandle();
	events[1] = _this->DisposeEvent.GetHandle();

	while(!_this->IsDisposing())
	{
		DWORD res = WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
		if (res==WAIT_OBJECT_0-0)
		{
			if (theApp!=NULL)
				theApp->OnNewCommand(_this->GetCommand());
			_this->CommandEvent.Reset();
		}
	}

	return 0;
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
	HANDLE eventHandle = CreateEvent(&sa, TRUE, FALSE, _T("Global\\BarbaTunnel_CommandEvent"));
	_IsAlreadyRunning = GetLastError()==ERROR_ALREADY_EXISTS;
	if (eventHandle==NULL) 
		BarbaLog(_T("Could not create Global\\BarbaTunnel_CommandEvent!\n"));
	CommandEvent.Attach(eventHandle);

	//start command monitor
	this->CommandMonitorThreadHandle = (HANDLE)_beginthreadex(NULL, 16000, CommandMonitorThread, (void*)this, 0, NULL);
}

void BarbaComm::SetWorkingState(size_t /*length*/, bool /*send*/)
{
	if (GetTickCount()-LastWorkingTick>BARBA_WorkingStateRefreshTime)
	{
		LastWorkingTick = GetTickCount();
		time_t curTime = 0;
		time(&curTime);
		TCHAR ctimeBuf[100];
		_stprintf_s(ctimeBuf, _T("%llu"), curTime);
		WritePrivateProfileString(_T("General"), _T("LastWorkTime"), ctimeBuf, GetCommFilePath());
	}
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
	static SimpleCriticalSection cs;
	SimpleLock lock(&cs);

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
		WriteFile(NotifyFileHandle, msgUtf8, (DWORD)strlen(msgUtf8), &writeLen, NULL);
		SetEndOfFile(NotifyFileHandle);
	}
	else
	{
		//if (GetFileSize(LogFileHandle, NULL)>this->MaxLogFilesize)
		//{
		//	SetFilePointer(NotifyFileHandle, 0, 0, FILE_BEGIN);
		//	SetEndOfFile(NotifyFileHandle); //reset if reach limit
		//}

		_tprintf_s(msg);
		_tprintf_s(_T("\r\n"));
		WriteFile(LogFileHandle, msgUtf8, (DWORD)strlen(msgUtf8), &writeLen, NULL);
		WriteFile(LogFileHandle, _T("\r\n"), 2, &writeLen, NULL);
		SetEndOfFile(LogFileHandle); //release cash
	}
}

bool BarbaComm::CreateFiles()
{
	CreateDirectory(GetWorkFolderPath(), NULL);

	//make sure anyone have access to allow barba monitor send message
	TCHAR cmd[MAX_PATH*2];
	_stprintf_s(cmd, _T("\"%s\" /e /g everyone:c /t"), GetWorkFolderPath());
	BarbaUtils::SimpleShellExecuteAndWait(_T("cacls.exe"), cmd, NULL, SW_HIDE);

	//Create files
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
	else if (_tcsicmp(_T("Stop"), buf)==0) return CommandStop;
	else if (_tcsicmp(_T("UpdateSettings"), buf)==0) return CommandUpdateSettings;
	else return CommandNone;
}

void BarbaLogImpl(int level, LPCTSTR format, va_list _ArgList)
{
	if (theApp!=NULL && level>theApp->LogLevel)
		return;
	bool notify = level==0;

	TCHAR* msg = new TCHAR[2000];
	TCHAR* msgTime = new TCHAR[2000];
	_vstprintf_s(msg, 2000, format, _ArgList);
	_vstprintf_s(msgTime, 2000, format, _ArgList);
	
	if (theApp!=NULL && !notify)
		sprintf_s(msgTime, 2000, _T("%s> %s"), BarbaUtils::GetTimeString(theApp->TimeZone).data(), msg);

	if (theComm!=NULL)
	{
		theComm->Log(msgTime, notify);
	}
	else
	{
		_tprintf_s(msg);
		_tprintf_s(_T("\r\n"));
	}
	delete msg;
	delete msgTime;
}

void BarbaLog(LPCTSTR format, ...) { va_list argp; va_start(argp, format); BarbaLogImpl(1, format, argp); va_end(argp); }
void BarbaLog1(LPCTSTR format, ...) { va_list argp; va_start(argp, format); BarbaLogImpl(1, format, argp); va_end(argp); }
void BarbaLog2(LPCTSTR format, ...) { va_list argp; va_start(argp, format); BarbaLogImpl(2, format, argp); va_end(argp); }
void BarbaLog3(LPCTSTR format, ...) { va_list argp; va_start(argp, format); BarbaLogImpl(3, format, argp); va_end(argp); }
void BarbaNotify(LPCTSTR format, ...) { 	va_list argp; va_start(argp, format); BarbaLogImpl(0, format, argp); va_end(argp); }

