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
BarbaApp* barbaApp;

bool CheckAdapterIndex()
{
	if (barbaApp->AdapterIndex-1>=0 && barbaApp->AdapterIndex-1<(int)AdList.m_nAdapterCount)
	{
		CurrentAdapterIndex = barbaApp->AdapterIndex-1;
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
	if (MessageBox(NULL, msg, _T("Barbatunnel"), MB_ICONWARNING|MB_YESNO)==IDYES)
	{
		BarbaUtils::SimpleShellExecute(barbaApp->GetConfigFile(), NULL, SW_SHOW, NULL, _T("edit"));
	}
	return false;
}

void SetMTU()
{
	api.SetMTUDecrement( barbaApp->GetMTUDecrement() ) ;

	LPCTSTR msg = 
		_T("Barbatunnel set new MTU decrement to have enough space for adding Barba header to your packet.\n\n")
		_T("You must restart Windows so the new MTU decrement can effect.\n\nDo you want to restart now?");

	if (MessageBox(NULL, msg, _T("Barabtunnel"), MB_ICONQUESTION|MB_YESNO)==IDYES)
	{
		BarbaUtils::SimpleShellExecute(_T("shutdown.exe"), _T("/r /t 0 /d p:4:2"), SW_HIDE);
	}
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


RAS_LINKS RasLinks;

int main(int argc, char* argv[])
{
	//set process priority
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	//check is driver loaded
	if(!api.IsDriverLoaded())
	{
		printf ("Driver not installed on this system of failed to load.\n");
		return 0;
	}

	//create BarbaApp
	for (int i=0; i<argc; i++)
	{
		if (_stricmp(argv[i], "/server")==0)
			IsBarbaServer = true;
	}
	barbaApp = IsBarbaServer ? (BarbaApp*)&barbaServerApp : (BarbaApp*)&barbaClientApp ;
	barbaApp->Init();

	//process other command line
	for (int i=0; i<argc; i++)
	{
		if (_stricmp(argv[i], "/debug")==0)
		{
			barbaApp->IsDebugMode = true;
		}
		else if (_stricmp(argv[i], "/setmtu")==0)
		{
			SetMTU();
			return 0;
		}
	}

	//try to set MTU as administrator
	if (barbaApp->GetMTUDecrement() > api.GetMTUDecrement()  )
	{
		TCHAR file[MAX_PATH];
		GetModuleFileName(NULL, file, _countof(file));
		printf ("Set new MTU decrement: %d\n", barbaApp->GetMTUDecrement());
		BarbaUtils::SimpleShellExecute(file, _T("/setmtu"), SW_HIDE, NULL, _T("runas"));
		return 0;
	}
	
	//get adapter list
	api.GetTcpipBoundAdaptersInfo ( &AdList );
	if (!CheckAdapterIndex())
		return 0;

	ADAPTER_MODE Mode;
	Mode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL;
	Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[CurrentAdapterIndex];

	// Create notification event
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Set event for helper driver
	if ((!hEvent)||(!api.SetPacketEvent((HANDLE)AdList.m_nAdapterHandle[CurrentAdapterIndex], hEvent)))
	{
		printf ("Failed to create notification event or set it for driver.\n");
		return 0;
	}

	atexit (ReleaseInterface);
	
	// Initialize Request
	barbaApp->CurrentRequest.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[CurrentAdapterIndex];
	api.SetAdapterMode(&Mode);

	//print info
	printf(_T("Barba %s Started\n"), IsBarbaServer ? _T("Server") : _T("Client"));
	printf(_T("Version: %d\n"), BARBA_CURRENT_VERSION);
	TCHAR adapterName[ADAPTER_NAME_SIZE];
	CNdisApi::ConvertWindows2000AdapterName((LPCTSTR)AdList.m_szAdapterNameList[CurrentAdapterIndex], adapterName, _countof(adapterName));
	printf(_T("Adpater Name: %s\n"), adapterName);


	bool terminate = false;
	while (!terminate)
	{
		WaitForSingleObject ( hEvent, INFINITE );
		while(api.ReadPacket(&barbaApp->CurrentRequest))
		{
			__try
			{
				PINTERMEDIATE_BUFFER buffer = barbaApp->CurrentRequest.EthPacket.Buffer;
				bool send = buffer->m_dwDeviceFlags == PACKET_FLAG_ON_SEND;

				//check commands
				if (barbaApp->CheckTerminateCommands(buffer))
					terminate = true;

				//process packet
				barbaApp->ProcessPacket(buffer);

				//send packet
				if (send)
					api.SendPacketToAdapter(&barbaApp->CurrentRequest);
				else
					api.SendPacketToMstcp(&barbaApp->CurrentRequest);
			}
			__except ( 0, EXCEPTION_EXECUTE_HANDLER) //catch all exception including system exception
			{
				_tprintf(_T("Application throw unhandled exception! packet dropped.\n"));
			}
		}
		ResetEvent(hEvent);
	}

	printf ("Filtering complete\n");
	return 0;
}

