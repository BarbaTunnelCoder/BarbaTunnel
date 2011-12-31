#include "stdafx.h"
#include "BarbaClient\BarbaClientApp.h"
#include "BarbaServer\BarbaServerApp.h"

TCP_AdapterList		AdList;
DWORD				CurrentAdapterIndex;
CNdisApi			api;
HANDLE				hEvent;

bool IsBarbaServer;
BarbaClientApp barbaClientApp;
BarbaServerApp barbaServerApp;
bool StartProcessPackets(HANDLE commandEventHandle, BarbaComm::CommandEnum& barbaCommandOut);
void ApplyServerPacketFilter();
void ApplyClientPacketFilter();
void ApplyPacketFilter();

bool CheckAdapterIndex()
{
	if (theApp->GetAdapterIndex()-1>=0 && theApp->GetAdapterIndex()-1<(int)AdList.m_nAdapterCount)
	{
		CurrentAdapterIndex = theApp->GetAdapterIndex()-1;
		return true;
	}

	//find best adapter
	TCHAR msg[ADAPTER_NAME_SIZE*ADAPTER_LIST_SIZE + 255] = {0};
	_tcscat_s(msg, _T("Could not find main network adapter!\nPlease set your main network adapter index in AdapterIndex of config.ini file.\n\n"));

	int findCount = 0;
	int findIndex = 0;
	for (size_t i=0; i<AdList.m_nAdapterCount; i++)
	{
		TCHAR adapterName[ADAPTER_NAME_SIZE];
		CNdisApi::ConvertWindows2000AdapterName((LPCTSTR)AdList.m_szAdapterNameList[i], adapterName, _countof(adapterName));

		TCHAR adapterNameLower[ADAPTER_NAME_SIZE] = {0};
		for(size_t j = 0; adapterName[j] != '\0' && i<ADAPTER_NAME_SIZE; j++)
			adapterNameLower[j] = (TCHAR)_totlower(adapterName[j]);

		bool isWan = _tcsstr(adapterNameLower, _T("wan network"))!=NULL;
		if (isWan)
			continue;

		//save last find
		findCount++;
		findIndex = i;

		//add adapter name
		TCHAR adapterLine[ADAPTER_NAME_SIZE + 10];
		_stprintf_s(adapterLine, _T("%d) %s"), i+1, adapterName);
		_tcscat_s(msg, adapterLine);
		_tcscat_s(msg, _T("\n"));
	}

	//use the only found adapter
	if (findCount==1)
	{
		CurrentAdapterIndex = findIndex;
		return true;
	}

	_tcscat_s(msg, _T("\nDo you want to open config.ini file now?"));
	if (MessageBox(NULL, msg, _T("BarbaTunnel"), MB_ICONWARNING|MB_YESNO)==IDYES)
	{
		BarbaUtils::SimpleShellExecute(theApp->GetConfigFile(), NULL, SW_SHOW, NULL, _T("edit"));
	}
	return false;
}

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


void ReleaseInterface()
{
	// This function releases packets in the adapter queue and stops listening the interface
	ADAPTER_MODE Mode;

	Mode.dwFlags = 0;
	Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[CurrentAdapterIndex];

	// Set NULL event to release previously set event object
	api.SetPacketEvent(AdList.m_nAdapterHandle[CurrentAdapterIndex], NULL);

	// Close Event
	if (hEvent)
		CloseHandle ( hEvent );

	// Set default adapter mode
	api.SetAdapterMode(&Mode);

	// Empty adapter packets queue
	api.FlushAdapterPacketQueue (AdList.m_nAdapterHandle[CurrentAdapterIndex]);
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

void test()
{
	printf( BarbaUtils::FormatTimeForHttp().data() );
}
int main(int argc, char* argv[])
{
	//test(); return 0;

	// memory leak detection
	InitMemoryLeackReport();

	//find IsBarbaServer
	IsBarbaServer = GetPrivateProfileInt(_T("General"), _T("ServerMode"), 0, BarbaApp::GetConfigFile())!=0;

	//create App
	try
	{
		theApp = IsBarbaServer ? (BarbaApp*)&barbaServerApp : (BarbaApp*)&barbaClientApp ;
		theApp->Initialize();

		//check is already running
		if (theApp->Comm.IsAlreadyRunning())
		{
			if (theApp!=NULL) theApp->Dispose();
			BarbaLog(_T("BarbaTunnel already running!"));
			return 0;
		}

		//try prepare Comm Files
		if (!theApp->Comm.CreateFiles() && !theApp->Comm.CreateFilesWithAdminPrompt())
			BarbaLog(_T("Could not prepare BarbaComm files!"));
	}
	catch(BarbaException* er)
	{
		BarbaLog(er->ToString());
		delete er;
		return 0;
	}

	//process other command line
	bool delayStart = false;
	for (int i=0; i<argc; i++)
	{
		if (_tcsicmp(argv[i], _T("/setmtu"))==0)
		{
			SetMTU();
			theApp->Comm.CreateFilesWithAdminPrompt();
			return 0;
		}
		else if (_tcsicmp(argv[i], _T("/delaystart"))==0 && IsBarbaServer)
		{
			delayStart = true;
		}
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
	if (delayStart && IsBarbaServer)
	{
		DWORD delayMin = theServerApp->AutoStartDelay;
		theApp->Comm.SetStatus(_T("Waiting"));
		BarbaLog(_T("Barba Server is waiting for AutoStartDelay (%d minutes)."), delayMin);
		BarbaNotify(_T("Barba Server is waiting\r\nBarba Server is waiting for %d minutes."), delayMin);
		Sleep(delayMin * 60* 1000);
	}

	//check is driver loaded (let after Comm Files created)
	if(!api.IsDriverLoaded())
	{
		BarbaLog(_T("Error: Driver not installed on this system or failed to load!\r\nPlease go to http://www.ntndis.com/w&p.php?id=7 and install WinpkFilter driver."));
		BarbaNotify(_T("Error: Driver not installed!\r\nDriver not installed on this system or failed to load!"));
		return 0;
	}

	//get adapter list
	api.GetTcpipBoundAdaptersInfo ( &AdList );
	if (!CheckAdapterIndex())
		return 0;

	//report info
	TCHAR adapterName[ADAPTER_NAME_SIZE];
	CNdisApi::ConvertWindows2000AdapterName((LPCTSTR)AdList.m_szAdapterNameList[CurrentAdapterIndex], adapterName, _countof(adapterName));
	LPCTSTR barbaName = IsBarbaServer ? _T("Barba Server") : _T("Barba Client");
	BarbaLog(_T("%s Started...\r\nVersion: %s\r\nAdapter: %s\r\nReady!"), barbaName, BARBA_CURRENT_VERSION, adapterName);
	BarbaNotify(_T("%s Started\r\nVersion: %s\r\nAdpater: %s"), barbaName, BARBA_CURRENT_VERSION, adapterName);
	theApp->Comm.SetStatus(_T("Started"));
	theApp->Start();

		//start process packets
	BarbaComm::CommandEnum barbaCommand = BarbaComm::CommandNone;
	if (!StartProcessPackets(commandEventHandle, barbaCommand))
		return 0;

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
		if (CreateProcess(BarbaApp::GetModuleFile(), _T(""), NULL, NULL, FALSE, 0, NULL, NULL, &inf, &pi))
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
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	ADAPTER_MODE Mode;
	Mode.dwFlags = MSTCP_FLAG_SENT_TUNNEL | MSTCP_FLAG_RECV_TUNNEL;
	Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[CurrentAdapterIndex];
	api.SetAdapterMode(&Mode);
	theApp->SetAdapterHandle( (HANDLE)AdList.m_nAdapterHandle[CurrentAdapterIndex] );
	ApplyPacketFilter(); //filter IP to optimize network

	// Create notification event
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	// Set event for helper driver
	if (!hEvent || !api.SetPacketEvent(Mode.hAdapterHandle, hEvent))
	{
		BarbaLog(_T("Failed to create notification event or set it for driver."));
		return false;
	}
	atexit (ReleaseInterface);

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
				__try
				{
					PINTERMEDIATE_BUFFER buffer = CurrentRequest.EthPacket.Buffer;
					bool send = buffer->m_dwDeviceFlags == PACKET_FLAG_ON_SEND;
					PacketHelper packet(buffer->m_IBuffer);

					//check commands
					if (theApp->IsDebugMode() && theApp->CheckTerminateCommands(&packet, send))
					{
						BarbaLog(_T("Terminate Command Received."));
						terminate = true;
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
				__except ( 0, EXCEPTION_EXECUTE_HANDLER) //catch all exception including system exception
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

void ApplyClientPacketFilter()
{
	size_t configItemCount = theClientApp->ConfigManager.Configs.size();
	size_t filterCount = configItemCount*2 + 1;
	std::vector<BYTE> filterTableBuf( sizeof STATIC_FILTER_TABLE  * filterCount );
	STATIC_FILTER_TABLE* filterTable = (STATIC_FILTER_TABLE*)&filterTableBuf.front();
	filterTable->m_TableSize = filterCount;

	for (size_t i=0; i<configItemCount; i++)
	{
		BarbaClientConfig* configItem = &theClientApp->ConfigManager.Configs[i];

		//redirect only packet that send to our server
		STATIC_FILTER* staticFilter = &filterTable->m_StaticFilters[i*2];
		staticFilter->m_Adapter.LowPart = (DWORD)theApp->GetAdapterHandle();
		staticFilter->m_FilterAction = FILTER_PACKET_REDIRECT;
		staticFilter->m_dwDirectionFlags = PACKET_FLAG_ON_SEND;
		staticFilter->m_ValidFields = NETWORK_LAYER_VALID;
		staticFilter->m_NetworkFilter.m_dwUnionSelector = IPV4;

		IP_V4_FILTER* filter = &staticFilter->m_NetworkFilter.m_IPv4;
		filter->m_ValidFields = IP_V4_FILTER_DEST_ADDRESS;
		filter->m_DestAddress.m_AddressType=IP_SUBNET_V4_TYPE;
		filter->m_DestAddress.m_IpSubnet.m_Ip = configItem->ServerIp;
		filter->m_DestAddress.m_IpSubnet.m_IpMask = 0xFFFFFFFF;

		//redirect only packet that receive from our server
		staticFilter = &filterTable->m_StaticFilters[i*2+1];
		staticFilter->m_Adapter.LowPart = (DWORD)theApp->GetAdapterHandle();
		staticFilter->m_FilterAction = FILTER_PACKET_REDIRECT;
		staticFilter->m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE;
		staticFilter->m_ValidFields = NETWORK_LAYER_VALID;
		staticFilter->m_NetworkFilter.m_dwUnionSelector = IPV4;

		filter = &staticFilter->m_NetworkFilter.m_IPv4;
		filter->m_ValidFields = IP_V4_FILTER_SRC_ADDRESS;
		filter->m_SrcAddress.m_AddressType=IP_SUBNET_V4_TYPE;
		filter->m_SrcAddress.m_IpSubnet.m_Ip = configItem->ServerIp;
		filter->m_SrcAddress.m_IpSubnet.m_IpMask = 0xFFFFFFFF;
	}

	//pass all other
	STATIC_FILTER* staticFilter = &filterTable->m_StaticFilters[filterCount-1];
	staticFilter->m_Adapter.LowPart = (DWORD)theApp->GetAdapterHandle();
	staticFilter->m_FilterAction = FILTER_PACKET_PASS;
	staticFilter->m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE | PACKET_FLAG_ON_SEND;
	staticFilter->m_ValidFields = 0;

	if (!api.SetPacketFilterTable(filterTable))
		BarbaLog(_T("Warning: Could not set packet filtering to optimize network performance!"));
}

void ApplyServerPacketFilter()
{
	size_t filterCount = 2;
	std::vector<BYTE> filterTableBuf( sizeof STATIC_FILTER_TABLE  * filterCount );
	STATIC_FILTER_TABLE* filterTable = (STATIC_FILTER_TABLE*)&filterTableBuf.front();
	filterTable->m_TableSize = filterCount;

	//process just IP packets
	STATIC_FILTER* staticFilter = &filterTable->m_StaticFilters[0];
	staticFilter->m_Adapter.LowPart = (DWORD)theApp->GetAdapterHandle();
	staticFilter->m_FilterAction = FILTER_PACKET_REDIRECT;
	staticFilter->m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE | PACKET_FLAG_ON_SEND;
	staticFilter->m_ValidFields = DATA_LINK_LAYER_VALID;
	staticFilter->m_DataLinkFilter.m_dwUnionSelector = ETH_802_3;
	staticFilter->m_DataLinkFilter.m_Eth8023Filter.m_ValidFields = ETH_802_3_PROTOCOL;
	staticFilter->m_DataLinkFilter.m_Eth8023Filter.m_Protocol = ETH_P_IP;

	//pass all other packets
	staticFilter = &filterTable->m_StaticFilters[1];
	staticFilter->m_Adapter.LowPart = (DWORD)theApp->GetAdapterHandle();
	staticFilter->m_FilterAction = FILTER_PACKET_PASS;
	staticFilter->m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE | PACKET_FLAG_ON_SEND;
	staticFilter->m_ValidFields = 0;

	if (!api.SetPacketFilterTable(filterTable))
		BarbaLog(_T("Warning: Could not set packet filtering to optimize network performance!"));
}

void ApplyPacketFilter()
{
	if (IsBarbaServer)
		ApplyServerPacketFilter();
	else
		ApplyClientPacketFilter();
}
