#include "StdAfx.h"
#include "BarbaApp.h"
#include "SimpleEvent.h"

BarbaFilterDriver* CreateFilterDriverByName(LPCTSTR name);
BarbaApp* theApp = NULL;

BarbaApp::BarbaApp(void)
{
	Comm.Initialize(); //initialize soon as possible to see errors
	BarbaSocket::InitializeLib();
	IsRestartCommand = false;
	_IsDisposed = false;
	LogAnonymously = true;
	FilterDriver = NULL;
	srand( (UINT)time(0) );
}

BarbaApp::~BarbaApp(void)
{
	BarbaSocket::UninitializeLib();
}

void BarbaApp::Load()
{

	_stprintf_s(_ConfigFile, _T("%s\\BarbaTunnel.ini"), GetAppFolder());
	_AdapterIndex = GetPrivateProfileInt(_T("General"), _T("AdapterIndex"), 0, GetSettingsFile());
	LogLevel = GetPrivateProfileInt(_T("General"), _T("LogLevel"), 1, GetSettingsFile());
	LogAnonymously = GetPrivateProfileInt(_T("General"), _T("LogAnonymously"), 1, GetSettingsFile())!=0;
	_DebugMode = GetPrivateProfileInt(_T("General"), _T("DebugMode"), 0, GetSettingsFile())!=0;
	ConnectionTimeout = GetPrivateProfileInt(_T("General"), _T("ConnectionTimeout"), 0, GetSettingsFile())*60*1000;
	MTUDecrement = GetPrivateProfileInt(_T("General"), _T("MTUDecrement"), GetMinMTUDecrement(), GetSettingsFile());
	if (ConnectionTimeout==0) ConnectionTimeout = BARBA_ConnectionTimeout;
	if (LogLevel<1) LogLevel = 1;

	//Read TimeZone
	TCHAR timeZoneBuf[200] = {0};
	GetPrivateProfileString(_T("General"), _T("TimeZone"), _T(""), timeZoneBuf, _countof(timeZoneBuf), GetSettingsFile());
	TimeZone = BarbaUtils::GetTimeZoneFromString(timeZoneBuf);
	
	//MaxLogFilesize
	//Comm.MaxLogFilesize = GetPrivateProfileInt(_T("General"), _T("MaxLogFileSize"), BARBA_MaxLogFileSize/1000, GetSettingsFile())*1000;
	//if (Comm.MaxLogFilesize==0) Comm.MaxLogFilesize = BARBA_MaxLogFileSize;

	//FakeFileHeaders
	InitFakeFileHeaders();

	//FilterDriver
	TCHAR filterDriverName[200] = {0};
	GetPrivateProfileString(_T("General"), _T("FilterDriver"), _T(""), filterDriverName, _countof(filterDriverName), GetSettingsFile());
	if (_tcslen(filterDriverName)==0)
		_tcscpy_s(filterDriverName, _T("WinDivert"));
	FilterDriver = CreateFilterDriverByName(filterDriverName);
}

void BarbaApp::Initialize()
{
	Load();
	BarbaLog(_T("%s Started..."), theApp->GetName());
	BarbaLog(_T("Version: %s"), BARBA_CurrentVersion);
	BarbaLog(_T("FilterDriver: %s"), theApp->GetFilterDriver()->GetName());
	FilterDriver->Initialize();
}

void BarbaApp::InitFakeFileHeaders()
{
	TCHAR folder[MAX_PATH];
	_stprintf_s(folder, _T("%s\\templates"), GetAppFolder());

	std::vector<std::tstring> files;
	BarbaUtils::FindFiles(folder, _T("*.header"), &files);
	for (size_t i=0; i<files.size(); i++)
	{
		TCHAR contentTypeFile[MAX_PATH];
		_stprintf_s(contentTypeFile, _T("%s\\ContentType.txt"), folder);

		FakeFileHeader ffh;
		//extension
		ffh.Extension = BarbaUtils::FindFileTitle(files[i].data());
		//contentType
		TCHAR contentType[MAX_PATH];
		GetPrivateProfileString(_T("General"), ffh.Extension.data(), ffh.Extension.data(), contentType, _countof(contentType), contentTypeFile);
		ffh.ContentType = contentType;
		//file-header
		if ( BarbaUtils::LoadFileToBuffer(files[i].data(), &ffh.Data) )
			FakeFileHeaders.append(ffh);
	}
}

void BarbaApp::AddThread(HANDLE threadHandle)
{
	Threads.AddTail(threadHandle);
	CloseFinishedThreadHandle(&Threads);
}

void BarbaApp::CloseFinishedThreadHandle(SimpleSafeList<HANDLE>* list)
{
	SimpleSafeList<HANDLE>::AutoLockBuffer autoLockBuf(list);
	HANDLE* handles = autoLockBuf.GetBuffer();
	for (size_t i=0; i<list->GetCount(); i++)
	{
		bool alive = false;
		if ( BarbaUtils::IsThreadAlive(handles[i], &alive) && !alive)
		{
			list->Remove(handles[i]);
			CloseHandle(handles[i]);
		}
	}
}

void BarbaApp::CloseSocketsList(SimpleSafeList<BarbaSocket*>* list)
{
	//IncomingSockets
	SimpleSafeList<BarbaSocket*>::AutoLockBuffer autoLockBuf(list);
	BarbaSocket** socketsArray = autoLockBuf.GetBuffer();
	for (size_t i=0; i<list->GetCount(); i++)
	{
		try
		{
			socketsArray[i]->Close();
		}
		catch(BarbaException* er)
		{
			delete er;
		}
	}
}

LPCTSTR BarbaApp::GetConfigFolder()
{
	static TCHAR configItemFolder[MAX_PATH] = {0};
	if (configItemFolder[0]==0)
	{
		_stprintf_s(configItemFolder, _T("%s\\config"), GetAppFolder(), BARBA_ConfigFolderName);
	}
	return configItemFolder;
}

LPCTSTR BarbaApp::GetAppFolder()
{
	static TCHAR file[MAX_PATH] = {0};
	if (file[0]==0)
	{
		std::tstring folder = BarbaUtils::GetFileFolderFromUrl(GetSettingsFile());
		_tcscpy_s(file, folder.data());
	}
	return file;
}

LPCTSTR BarbaApp::GetSettingsFile()
{
	static TCHAR file[MAX_PATH] = {0};
	if (file[0]!=0)
		return file;

	std::tstring exeFolder = BarbaUtils::GetModuleFolder();
	std::tstring binFolder = BarbaUtils::GetFileFolderFromUrl(exeFolder.data());

	std::tstring appFolder = BarbaUtils::GetFileFolderFromUrl(binFolder.data());
	_stprintf_s(file, _T("%s\\BarbaTunnel.ini"), appFolder.data()); 
	if (BarbaUtils::IsFileExists(file))
		return file;

	//maybe we are in configuration folder in development environment
	appFolder = BarbaUtils::GetFileFolderFromUrl(appFolder.data());
	_stprintf_s(file, _T("%s\\BarbaTunnel.ini"), appFolder.data());
	if (BarbaUtils::IsFileExists(file))
		return file;

	file[0] = 0;
	return file;
}

LPCTSTR BarbaApp::GetModuleFile()
{
	static TCHAR moduleFile[MAX_PATH] = {0};
	if (moduleFile[0]==0)
	{
		::GetModuleFileName(NULL, moduleFile, MAX_PATH);
	}
	return moduleFile;
}

LPCTSTR BarbaApp::GetModuleFolder()
{
	static TCHAR file[MAX_PATH] = {0};
	if (file[0]==0)
	{
		std::tstring folder = BarbaUtils::GetModuleFolder();
		_tcscpy_s(file, folder.data());

	}
	return file;
}

int BarbaApp::GetMinMTUDecrement()
{
	return sizeof iphdr + sizeof udphdr + GetBarbaTag().size() + 20; //20 for chunk header
}
 
bool BarbaApp::CheckTerminateCommands(PacketHelper* packet, bool outbound)
{
	if (outbound || !packet->IsIp())
		return false;

	if (packet->ipHeader->ip_p!=1)
		return false;

	size_t nlen = packet->GetIpLen();
	size_t code = nlen - 28;
	if (code==1350)
		return true;

	return false;
}

void BarbaApp::Dispose()
{
	//check is already disposed
	if (_IsDisposed)
		return; 

	//dispose
	_IsDisposed = true;
	Stop();

	//wait for all thread to end
	HANDLE thread = Threads.RemoveHead();
	while (thread!=NULL)
	{
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		thread = Threads.RemoveHead();
	}

	//dispose Comm
	Comm.Dispose();
	FilterDriver->Dispose();
	delete FilterDriver;
}

bool BarbaApp::SendPacketToOutbound(PacketHelper* packet)
{
	return FilterDriver->SendPacketToOutbound(packet);
}

bool BarbaApp::SendPacketToInbound(PacketHelper* packet)
{
	return FilterDriver->SendPacketToInbound(packet);
}

bool BarbaApp::CheckMTUDecrement(size_t outgoingPacketLength, u_short requiredMTUDecrement)
{
	static bool ShowMtuError = true;
	bool ret = (outgoingPacketLength + requiredMTUDecrement)<=FilterDriver->GetMaxPacketLen();
	if ( !ret && ShowMtuError)
	{
		ShowMtuError = false;
		if (FilterDriver->GetMTUDecrement()<requiredMTUDecrement)
		{
			BarbaLog(_T("Error: Large outgoing packet size! Your current MTU-Decrement is %d, Please set MTU-Decrement to %d in BarbaTunnel.ini."), FilterDriver->GetMTUDecrement(), requiredMTUDecrement);
			BarbaNotify(_T("Error: Large outgoing packet size!\r\nPlease set MTU-Decrement to %d in BarbaTunnel.ini."), requiredMTUDecrement);
		}
		else
		{
			BarbaLog(_T("Error: Large outgoing packet size! More %d MTU-Decrement bytes required, Are you sure your system has been restarted?"), outgoingPacketLength + requiredMTUDecrement - FilterDriver->GetMaxPacketLen());
			BarbaNotify(_T("Error: Large outgoing packet size!\r\nAre you sure your system has been restarted?"));
		}
	}

	return ret;
}

bool BarbaApp::GetFakeFile(BarbaArray<std::tstring>* fakeTypes, size_t fakeFileMaxSize, TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew)
{
	if (fileSize!=NULL)
		*fileSize = BarbaUtils::GetRandom((u_int)fakeFileMaxSize/2, (u_int)fakeFileMaxSize); 

	//generate new filename in createNew
	if (createNew)
	{
		u_int fileNameId = BarbaUtils::GetRandom(1, UINT_MAX);
		TCHAR fileNameIdStr[MAX_PATH];
		_ltot_s(fileNameId, fileNameIdStr, MAX_PATH, 32);
		_tcscpy_s(filename, MAX_PATH, fileNameIdStr);
		
		if (!fakeTypes->empty())
		{
			int index = BarbaUtils::GetRandom(0, (u_int)fakeTypes->size()-1);
			_stprintf_s(filename, MAX_PATH, _T("%s.%s"), fileNameIdStr, fakeTypes->at(index).data());
		}
	}

	//find extension and fill fakeFileHeader
	*contentType = BarbaUtils::GetFileExtensionFromUrl(filename);
	if (fakeFileHeader!=NULL)
	{
		std::tstring extension = BarbaUtils::GetFileExtensionFromUrl(filename);
		for (size_t i=0; i<FakeFileHeaders.size(); i++)
		{
			if (_tcsicmp(FakeFileHeaders[i].Extension.data(), extension.data())==0)
			{
				*contentType = FakeFileHeaders[i].ContentType;
				*fakeFileHeader = FakeFileHeaders[i].Data;
			}
		}
	}

	return true;
}

void BarbaApp::UpdateSettings()
{
	LogLevel = GetPrivateProfileInt(_T("General"), _T("LogLevel"), 0, GetSettingsFile());
}

void BarbaApp::OnNewCommand(BarbaComm::CommandEnum command)
{
	switch(command){
	case BarbaComm::CommandRestart:
		IsRestartCommand = true;
		Stop();
		break;

	case BarbaComm::CommandStop:
		Stop();
		break;

	case BarbaComm::CommandUpdateSettings:
		theApp->UpdateSettings();
		break;
	}
}

void BarbaApp::Stop()
{
	if (FilterDriver->IsStopping() || !FilterDriver->IsStarted())
		return;

	BarbaLog(_T("BarbaTunnel Stopping."));
	if (FilterDriver!=NULL)
		FilterDriver->Stop();
}

void BarbaApp::Start()
{
	//set current process priority to process network packets as fast as possible
	if (!theApp->IsDebugMode())
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	//start FilterDriver
	BarbaLog(_T("Ready!"));
	BarbaNotify(_T("%s Started\r\nVersion: %s"), GetName(), BARBA_CurrentVersion);
	Comm.SetStatus(_T("Started"));
	FilterDriver->Start();

	//report finish
	BarbaLog(_T("BarbaTunnel Stopped."));
	Comm.SetStatus(_T("Stopped"));

	//report restarting
	PROCESS_INFORMATION pi = {0};
	if (IsRestartCommand)
	{
		BarbaLog(_T("BarbaTunnel Restarting..."));
		STARTUPINFO inf = {0};
		inf.cb = sizeof STARTUPINFO;
		GetStartupInfo(&inf);
		if (!CreateProcess(BarbaApp::GetModuleFile(), _T(""), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &inf, &pi))
			BarbaLog(_T("Failed to restart BarbaTunnel!"));
	}
	else
	{
		BarbaNotify(_T("%s Stopped\r\nBarbaTunnel Stopped"), GetName());
	}

	//cleanup
	Dispose();

	//restarting after dispose
	if (pi.hThread!=NULL)
	{
		ResumeThread(pi.hThread);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

bool BarbaApp::ProcessFilterDriverPacket(PacketHelper* packet, bool outbound)
{
	if (!packet->IsIp())
		return false;

	//check commands
	if (IsDebugMode() && CheckTerminateCommands(packet, outbound))
	{
		BarbaLog(_T("Terminate Command Received."));
		Stop();
		return true;
	}

	if (outbound)
		return ProcessOutboundPacket(packet);
	else
		return ProcessInboundPacket(packet);
}
