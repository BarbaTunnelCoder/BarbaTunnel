#include "StdAfx.h"
#include "BarbaApp.h"

BarbaApp* theApp = NULL;
extern CNdisApi	api;

BarbaApp::BarbaApp(void)
{
	_IsDisposed = false;
	srand((UINT)time(0));
	_stprintf_s(_ConfigFile, _T("%s\\BarbaTunnel.ini"), GetModuleFolder());

	this->_AdapterIndex = GetPrivateProfileInt(_T("General"), _T("AdapterIndex"), 0, GetConfigFile());
	this->_VerboseMode = GetPrivateProfileInt(_T("General"), _T("VerboseMode"), 0, GetConfigFile())!=0;
	this->_DebugMode = GetPrivateProfileInt(_T("General"), _T("DebugMode"), 0, GetConfigFile())!=0;
	this->ConnectionTimeout = GetPrivateProfileInt(_T("General"), _T("ConnectionTimeout"), BARBA_ConnectionTimeout, GetConfigFile())*60*1000;
	this->MTUDecrement = GetPrivateProfileInt(_T("General"), _T("MTUDecrement"), -1, GetConfigFile());
	
	//ConnectionTimeout
	if (this->ConnectionTimeout==0) this->ConnectionTimeout = BARBA_ConnectionTimeout*60*1000;
	
	//MaxLogFilesize
	this->Comm.MaxLogFilesize = GetPrivateProfileInt(_T("General"), _T("MaxLogFileSize"), BARBA_MaxLogFileSize, GetConfigFile())*1000;
	if (this->Comm.MaxLogFilesize==0) this->Comm.MaxLogFilesize = BARBA_MaxLogFileSize*1000;

	//FakeFileHeaders
	InitFakeFileHeaders();
	
	BarbaSocket::InitializeLib();
}

void BarbaApp::InitFakeFileHeaders()
{
	TCHAR folder[MAX_PATH];
	_stprintf_s(folder, _T("%s\\templates"), GetModuleFolder());

	std::vector<std::tstring> files;
	BarbaUtils::FindFiles(folder, _T("*.header"), &files);
	for (int i=0; i<(int)files.size(); i++)
	{
		FakeFileHeader* ffh = new FakeFileHeader();
		ffh->Extension = BarbaUtils::FindFileName(files[i].data());
		if ( BarbaUtils::LoadFileToBuffer(files[i].data(), &ffh->Data) )
			this->FakeFileHeaders.AddTail(ffh);
		else
			delete ffh;
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

	int nlen = packet->GetIpLen();
	int code = nlen - 28;
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

	//delete FakeFileHeader objects
	FakeFileHeader* ffh = this->FakeFileHeaders.RemoveHead();
	while (ffh!=NULL)
	{
		delete ffh;
		ffh = this->FakeFileHeaders.RemoveHead();
	}

}

void BarbaApp::Initialize()
{
	//Comm
	Comm.Initialize(_VerboseMode ? 2 : 0);
}

void BarbaApp::Start()
{
}

bool BarbaApp::SendPacketToMstcp(PacketHelper* packet)
{
	INTERMEDIATE_BUFFER intBuf = {0};
	intBuf.m_dwDeviceFlags = PACKET_FLAG_ON_RECEIVE;
	intBuf.m_Length = min(MAX_ETHER_FRAME, packet->GetPacketLen());
	memcpy_s(intBuf.m_IBuffer, MAX_ETHER_FRAME, packet->GetPacket(), intBuf.m_Length);

	ETH_REQUEST req;
	req.hAdapterHandle = this->AdapterHandle;
	req.EthPacket.Buffer = &intBuf;
	return api.SendPacketToMstcp(&req)!=FALSE;
}

bool BarbaApp::SendPacketToAdapter(PacketHelper* packet)
{
	INTERMEDIATE_BUFFER intBuf = {0};
	intBuf.m_dwDeviceFlags = PACKET_FLAG_ON_SEND;
	intBuf.m_Length = min(MAX_ETHER_FRAME, packet->GetPacketLen());
	memcpy_s(intBuf.m_IBuffer, MAX_ETHER_FRAME, packet->GetPacket(), intBuf.m_Length);

	ETH_REQUEST req;
	req.hAdapterHandle = this->AdapterHandle;
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
