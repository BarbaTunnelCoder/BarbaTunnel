#include "StdAfx.h"
#include "BarbaApp.h"

BarbaApp* theApp = NULL;
extern CNdisApi	api;

BarbaApp::BarbaApp(void)
{
	_AdapterHandle = 0;
	_IsDisposed = false;
	srand((UINT)time(0));
	_stprintf_s(_ConfigFile, _T("%s\\BarbaTunnel.ini"), GetModuleFolder());

	this->_AdapterIndex = GetPrivateProfileInt(_T("General"), _T("AdapterIndex"), 0, GetConfigFile());
	this->VerboseMode = GetPrivateProfileInt(_T("General"), _T("VerboseMode"), 0, GetConfigFile())!=0;
	this->_DebugMode = GetPrivateProfileInt(_T("General"), _T("DebugMode"), 0, GetConfigFile())!=0;
	this->ConnectionTimeout = GetPrivateProfileInt(_T("General"), _T("ConnectionTimeout"), 0, GetConfigFile())*60*1000;
	this->MTUDecrement = GetPrivateProfileInt(_T("General"), _T("MTUDecrement"), -1, GetConfigFile());
	
	//ConnectionTimeout
	if (this->ConnectionTimeout==0) this->ConnectionTimeout = BARBA_ConnectionTimeout;
	
	//MaxLogFilesize
	this->Comm.MaxLogFilesize = GetPrivateProfileInt(_T("General"), _T("MaxLogFileSize"), BARBA_MaxLogFileSize/1000, GetConfigFile())*1000;
	if (this->Comm.MaxLogFilesize==0) this->Comm.MaxLogFilesize = BARBA_MaxLogFileSize;

	//FakeFileHeaders
	InitFakeFileHeaders();
	
	BarbaSocket::InitializeLib();
}

void BarbaApp::SetAdapterHandle(HANDLE adapterHandle)
{
	this->_AdapterHandle = adapterHandle;
}

void BarbaApp::InitFakeFileHeaders()
{
	TCHAR folder[MAX_PATH];
	_stprintf_s(folder, _T("%s\\templates"), GetModuleFolder());

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
			this->FakeFileHeaders.push_back(ffh);
	}
}

BarbaApp::~BarbaApp(void)
{
	BarbaSocket::UninitializeLib();
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

LPCTSTR BarbaApp::GetConfigItemFolder()
{
	static TCHAR configItemFolder[MAX_PATH] = {0};
	if (configItemFolder[0]==0)
	{
		_stprintf_s(configItemFolder, _T("%s\\config"), GetModuleFolder());
	}
	return configItemFolder;
}


LPCTSTR BarbaApp::GetConfigFile()
{
	static TCHAR configFile[MAX_PATH] = {0};
	if (configFile[0]==0)
	{
		_stprintf_s(configFile, _T("%s\\BarbaTunnel.ini"), GetModuleFolder());
	}
	return configFile;
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
	static TCHAR moduleFolder[MAX_PATH] = {0};
	if (moduleFolder[0]==0)
	{
		BarbaUtils::GetModuleFolder(moduleFolder);
	}
	return moduleFolder;
}


bool BarbaApp::CheckTerminateCommands(PacketHelper* packet, bool send)
{
	if (!send || !packet->IsIp())
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
	_IsDisposed = true;

	//wait for all thread to end
	HANDLE thread = this->Threads.RemoveHead();
	while (thread!=NULL)
	{
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		thread = this->Threads.RemoveHead();
	}

	//dispose Comm
	Comm.Dispose();
}

void BarbaApp::Initialize()
{
	//Comm
	Comm.Initialize();
}

void BarbaApp::Start()
{
}

bool BarbaApp::SendPacketToMstcp(PacketHelper* packet)
{
	INTERMEDIATE_BUFFER intBuf = {0};
	intBuf.m_dwDeviceFlags = PACKET_FLAG_ON_RECEIVE;
	intBuf.m_Length = min(MAX_ETHER_FRAME, (ULONG)packet->GetPacketLen());
	memcpy_s(intBuf.m_IBuffer, MAX_ETHER_FRAME, packet->GetPacket(), intBuf.m_Length);

	ETH_REQUEST req;
	req.hAdapterHandle = _AdapterHandle;
	req.EthPacket.Buffer = &intBuf;
	return api.SendPacketToMstcp(&req)!=FALSE;
}

bool BarbaApp::SendPacketToAdapter(PacketHelper* packet)
{
	INTERMEDIATE_BUFFER intBuf = {0};
	intBuf.m_dwDeviceFlags = PACKET_FLAG_ON_SEND;
	intBuf.m_Length = min(MAX_ETHER_FRAME, (ULONG)packet->GetPacketLen());
	memcpy_s(intBuf.m_IBuffer, MAX_ETHER_FRAME, packet->GetPacket(), intBuf.m_Length);

	ETH_REQUEST req;
	req.hAdapterHandle = this->_AdapterHandle;
	req.EthPacket.Buffer = &intBuf;
	return api.SendPacketToAdapter(&req)!=FALSE;
}

bool BarbaApp::CheckMTUDecrement(size_t outgoingPacketLength, u_short requiredMTUDecrement)
{
	static bool ShowMtuError = true;
	bool ret = (outgoingPacketLength + requiredMTUDecrement)<=MAX_ETHER_FRAME;
	if ( !ret && ShowMtuError)
	{
		ShowMtuError = false;
		if (api.GetMTUDecrement()<requiredMTUDecrement)
		{
			BarbaLog(_T("Error: Large outgoing packet size! Your current MTU-Decrement is %d, Please set MTU-Decrement to %d in BarbaTunnel.ini."), api.GetMTUDecrement(), requiredMTUDecrement);
			BarbaNotify(_T("Error: Large outgoing packet size!\r\nPlease set MTU-Decrement to %d in BarbaTunnel.ini."), requiredMTUDecrement);
		}
		else
		{
			BarbaLog(_T("Error: Large outgoing packet size! More %d MTU-Decrement bytes required, Are you sure your system has been restarted?"), MAX_ETHER_FRAME-outgoingPacketLength);
			BarbaNotify(_T("Error: Large outgoing packet size!\r\nAre you sure your system has been restarted?"));
		}
	}

	return ret;
}

bool BarbaApp::GetFakeFile(std::vector<std::tstring>* fakeTypes, size_t fakeFileMaxSize, TCHAR* filename, std::tstring* contentType, size_t* fileSize, std::vector<BYTE>* fakeFileHeader, bool createNew)
{
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
		for (size_t i=0; i<this->FakeFileHeaders.size(); i++)
		{
			if (_tcsicmp(this->FakeFileHeaders[i].Extension.data(), extension.data())==0)
			{
				*contentType = this->FakeFileHeaders[i].ContentType;
				*fakeFileHeader = this->FakeFileHeaders[i].Data;
			}
		}
	}

	return true;
}

void BarbaApp::UpdateSettings()
{
	this->VerboseMode = GetPrivateProfileInt(_T("General"), _T("VerboseMode"), 0, GetConfigFile())!=0;
}