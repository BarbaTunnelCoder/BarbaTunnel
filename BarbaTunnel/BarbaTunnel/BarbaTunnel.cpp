#include "stdafx.h"
#include "BarbaClient\BarbaClientApp.h"
#include "BarbaServer\BarbaServerApp.h"
#include "WinpkPacketFilter.h"

BarbaPacketFilter* CreatePacketFilterByName(LPCTSTR name)
{
	if (_tcsicmp(name, _T("WinpkFilter"))==0)
		return new WinpkPacketFilter();

	throw new BarbaException(_T("Invalid PacketFilter: %s", name));
}

bool StartProcessPackets(HANDLE commandEventHandle, BarbaComm::CommandEnum& barbaCommandOut);
bool SetMTU()
{
	if (theApp->GetMTUDecrement()==-1)
	{
		BarbaLog(_T("MTUDecrement is not set!"));
		return false;
	}

	api.SetMTUDecrement( theApp->GetMTUDecrement() ) ;
	if ((int)api.GetMTUDecrement()!=theApp->GetMTUDecrement())
		return false;

	LPCTSTR msg = 
		_T("BarbaTunnel set new MTU decrement to have enough space for adding Barba header to your packet.\n\n")
		_T("You must restart Windows so the new MTU decrement can effect.\n\nDo you want to restart now?");

	if (MessageBox(NULL, msg, _T("Barabtunnel"), MB_ICONQUESTION|MB_YESNO)==IDYES)
	{
		BarbaUtils::SimpleShellExecute(_T("shutdown.exe"), _T("/r /t 0 /d p:4:2"), SW_HIDE);
	}

	return true;
}


void InitMemoryLeackReport()
{
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );
}

int test()
{
	std::tstring folder = BarbaUtils::GetFileFolderFromUrl("d:\\mmm\\sss\\bb.txt\\");
	std::tstring folderName = BarbaUtils::GetFileNameFromUrl(folder.data());
	printf("%s\n%s\n", folder.data(), folderName.data());
	return 0;
}

int main(int argc, char* argv[])
{
	//test(); return 0; //just for debug
	//TODO: check msgbox in service //mad

	// memory leak detection
	InitMemoryLeackReport();
	try
	{
		//find IsBarbaServer
		bool isBarbaServer = GetPrivateProfileInt(_T("General"), _T("ServerMode"), 0, BarbaApp::GetSettingsFile())!=0;

		//find command line parameter
		bool delayStart = false;
		for (int i=0; i<argc; i++)
		{
			if (_tcsicmp(argv[i], _T("/delaystart"))==0)
				delayStart = true;
		}

		//create App
		theApp = isBarbaServer ? (BarbaApp*)new BarbaServerApp() : (BarbaApp*)new BarbaClientApp() ;
		theApp->Initialize();
	}
	catch(BarbaException* er)
	{
		BarbaLog(_T("Error: %s"), er->ToString());
		theApp->Dispose();
		delete er;
		delete theApp;
		return 0;
	}


	//try to set MTU as administrator
	if (theApp->GetMTUDecrement()!=-1 && theApp->GetMTUDecrement() != (int)api.GetMTUDecrement()  )
	{
		BarbaLog(_T("Trying to set new MTU decrement to %d."), theApp->GetMTUDecrement());
		if (!SetMTU())
		{
			theApp->Dispose();
			BarbaLog(_T("Could not set new MTU decrement. Retrying with administrator prompt."));
			if (!BarbaUtils::SimpleShellExecute(BarbaApp::GetModuleFile(), _T("/setmtu"), SW_HIDE, NULL, _T("runas")))
				BarbaLog(_T("Error: Failed to run %s with administrator prompt!"), BarbaUtils::GetFileNameFromUrl(BarbaApp::GetModuleFile()).data() );
		}
		return 0;
	}

	//create command listener event
	SECURITY_DESCRIPTOR sd = { 0 };
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	HANDLE commandEventHandle = CreateEvent(&sa, FALSE, FALSE, _T("Global\\BarbaTunnel_CommandEvent"));
	if (commandEventHandle==NULL)
	{
		BarbaLog(_T("Could not create Global\\BarbaTunnel_CommandEvent!"));
		return 0;
	}

	//wait for server
	if (delayStart && theApp->IsServerMode())
	{
		DWORD delayMin = theServerApp->AutoStartDelay;
		theApp->Comm.SetStatus(_T("Waiting"));
		BarbaLog(_T("Barba Server is waiting for AutoStartDelay (%d minutes)."), delayMin);
		BarbaNotify(_T("Barba Server is waiting\r\nBarba Server is waiting for %d minutes."), delayMin);
		Sleep(delayMin * 60* 1000);
	}

	//report info
	TCHAR adapterName[ADAPTER_NAME_SIZE];
	CNdisApi::ConvertWindows2000AdapterName((LPCTSTR)AdList.m_szAdapterNameList[CurrentAdapterIndex], adapterName, _countof(adapterName));
	LPCTSTR barbaName = theApp->IsServerMode() ? _T("Barba Server") : _T("Barba Client");
	BarbaLog(_T("%s Started...\r\nVersion: %s\r\nAdapter: %s\r\nReady!"), barbaName, BARBA_CurrentVersion, adapterName);
	BarbaNotify(_T("%s Started\r\nVersion: %s\r\nAdapter: %s"), barbaName, BARBA_CurrentVersion, adapterName);
	theApp->Comm.SetStatus(_T("Started"));
	theApp->Start();

	//start process packets
	BarbaComm::CommandEnum barbaCommand = BarbaComm::CommandNone;
	StartProcessPackets(commandEventHandle, barbaCommand);

	//report finish
	BarbaLog(_T("BarbaTunnel Stopped."));
	if (theApp!=NULL)
		theApp->Comm.SetStatus(_T("Stopped"));

	//report restarting
	HANDLE newThreadHandle = NULL;
	if (barbaCommand==BarbaComm::CommandRestart)
	{
		BarbaLog(_T("BarbaTunnel Restarting..."));
		STARTUPINFO inf = {0};
		inf.cb = sizeof STARTUPINFO;
		GetStartupInfo(&inf);
		PROCESS_INFORMATION pi = {0};
		if (CreateProcess(BarbaApp::GetModuleFile(), _T(""), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &inf, &pi))
			newThreadHandle = pi.hThread;
		else
			BarbaLog(_T("Failed to restart BarbaTunnel!"));
	}
	else
	{
		BarbaNotify(_T("%s Stopped\r\nBarbaTunnel Stopped"), barbaName);
	}

	//cleanup
	theApp->Dispose();
	CloseHandle(commandEventHandle);

	//restarting after dispose
	if (newThreadHandle!=NULL)
		ResumeThread(newThreadHandle);
	return 0;
}

bool StartProcessPackets(HANDLE commandEventHandle, BarbaComm::CommandEnum& barbaCommand)
{
	//set current process priority to process network packets as fast as possible
	if (!theApp->IsDebugMode())
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	ADAPTER_MODE Mode;
	Mode.dwFlags = MSTCP_FLAG_SENT_TUNNEL | MSTCP_FLAG_RECV_TUNNEL;
	Mode.hAdapterHandle = AdList.m_nAdapterHandle[CurrentAdapterIndex];
	theApp->SetAdapterHandle( AdList.m_nAdapterHandle[CurrentAdapterIndex] );
	if (!WinpkPacketFilter::ApplyPacketFilter())
		return false;
	api.SetAdapterMode(&Mode);

	// Create notification event
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	// Set event for helper driver
	if (!hEvent || !api.SetPacketEvent(Mode.hAdapterHandle, hEvent))
	{
		BarbaLog(_T("Failed to create notification event or set it for driver."));
		return false;
	}

	// Initialize Request
	ETH_REQUEST CurrentRequest = {0};
	INTERMEDIATE_BUFFER CurrentPacketBuffer = {0};
	CurrentRequest.hAdapterHandle = Mode.hAdapterHandle;
	CurrentRequest.EthPacket.Buffer = &CurrentPacketBuffer;

	//Handle
	HANDLE events[2];
	events[0] = hEvent;
	events[1] = commandEventHandle;

	bool terminate = false;
	while (!terminate)
	{
		DWORD res = WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
		if (res==WAIT_OBJECT_0-0)
		{
			while(api.ReadPacket(&CurrentRequest))
			{
				try
				{
					PINTERMEDIATE_BUFFER buffer = CurrentRequest.EthPacket.Buffer;
					bool send = buffer->m_dwDeviceFlags == PACKET_FLAG_ON_SEND;
					PacketHelper packet((ether_header_ptr)buffer->m_IBuffer);

					//check commands
					if (theApp->IsDebugMode() && theApp->CheckTerminateCommands(&packet, send))
					{
						BarbaLog(_T("Terminate Command Received."));
						terminate = true;
						break;
					}

					//process packet
					if (!theApp->ProcessPacket(&packet, send))
					{
						//send packet
						if (send)
							api.SendPacketToAdapter(&CurrentRequest);
						else
							api.SendPacketToMstcp(&CurrentRequest);
					}
				}
				catch(...)
				{
					BarbaLog(_T("Application throw unhandled exception! packet dropped.\n"));
				}
			}
			ResetEvent(hEvent);
		}
		else 
		{
			barbaCommand = theApp->Comm.GetCommand();
			if (barbaCommand==BarbaComm::CommandRestart || barbaCommand==BarbaComm::CommandStop)
				terminate = true;
			else if (barbaCommand==BarbaComm::CommandUpdateSettings)
				theApp->UpdateSettings();
			ResetEvent(commandEventHandle);
		}
	}

	return true;
}
