#include "stdafx.h"
#include "BarbaClient\BarbaClientApp.h"
#include "BarbaServer\BarbaServerApp.h"

TCP_AdapterList		AdList;
DWORD				iIndex;
CNdisApi			api;
HANDLE				hEvent;

void printPacketInfo(ether_header_ptr ethHeader);



void ReleaseInterface()
{
	// This function releases packets in the adapter queue and stops listening the interface
	ADAPTER_MODE Mode;

	Mode.dwFlags = 0;
	Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[iIndex];

	// Set NULL event to release previously set event object
	api.SetPacketEvent(AdList.m_nAdapterHandle[iIndex], NULL);

	// Close Event
	if (hEvent)
		CloseHandle ( hEvent );

	// Set default adapter mode
	api.SetAdapterMode(&Mode);

	// Empty adapter packets queue
	api.FlushAdapterPacketQueue (AdList.m_nAdapterHandle[iIndex]);
}

void Test(ULONG index, int counter)
{
	ETH_REQUEST			Request;
	INTERMEDIATE_BUFFER PacketBuffer;

	if(!api.IsDriverLoaded())
	{
		printf ("Driver not installed on this system of failed to load.\n");
		return;
	}
	
	api.GetTcpipBoundAdaptersInfo ( &AdList );

	if ( index + 1 > AdList.m_nAdapterCount )
	{
		printf("There is no network interface with such index on this system.\n");
		return;
	}

	
	ADAPTER_MODE Mode;
	Mode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL;
	Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[index];

	// Create notification event
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Set event for helper driver
	if ((!hEvent)||(!api.SetPacketEvent((HANDLE)AdList.m_nAdapterHandle[index], hEvent)))
	{
		printf ("Failed to create notification event or set it for driver.\n");
		return;
	}

	atexit (ReleaseInterface);
	
	// Initialize Request
	ZeroMemory ( &Request, sizeof(ETH_REQUEST) );
	ZeroMemory ( &PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	Request.EthPacket.Buffer = &PacketBuffer;
	Request.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[index];
		
	api.SetAdapterMode(&Mode);

	while (counter != 0)
	{
		WaitForSingleObject ( hEvent, INFINITE );
		
		while(api.ReadPacket(&Request))
		{
			bool send = PacketBuffer.m_dwDeviceFlags == PACKET_FLAG_ON_SEND;
			//ether_header_ptr ethHeader = (ether_header*)Request.EthPacket.Buffer->m_IBuffer;

			counter--;
			if (counter%100==0) 
				printf("Counter:%d\n", counter);

			if (send)
			{
				// Place packet on the network interface
				api.SendPacketToAdapter(&Request);
			}
			else
			{
				// Indicate packet to MSTCP
				api.SendPacketToMstcp(&Request);
			}

			if (counter == 0)
			{
				printf ("Filtering complete\n");
				break;
			}
		}

		ResetEvent(hEvent);
	}
}


bool IsBarbaServer;
BarbaClientApp barbaClientApp;
BarbaServerApp barbaServerApp;
BarbaApp* barbaApp;
int main(int argc, char* argv[])
{

	//set process priortiy
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

	if (argc < 2)
	{
		printf ("Command line syntax:\n\tGreTunnel.exe index num\n\tindex - network interface index.\n\tYou can use ListAdapters to determine correct index.\n");
		return 0;
	}

	iIndex = atoi(argv[1]) - 1;

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
			barbaApp->IsDebugMode = true;
	}


	if(!api.IsDriverLoaded())
	{
		printf ("Driver not installed on this system of failed to load.\n");
		return 0;
	}
	
	api.GetTcpipBoundAdaptersInfo ( &AdList );

	if ( iIndex + 1 > AdList.m_nAdapterCount )
	{
		printf("There is no network interface with such index on this system.\n");
		return 0;
	}

	DWORD dwMTUDec = api.GetMTUDecrement();
	DWORD myOverHead = sizeof iphdr + sizeof tcphdr + sizeof BarbaHeader;
	if (myOverHead > dwMTUDec )
	{
		api.SetMTUDecrement(myOverHead);
		printf ("Incorrect MTU decrement was set for the system. New MTU decrement is %d bytes. Please reboot the system for the changes to take the effect.\n", myOverHead);
		//return 0;
	}
	printf ("MTU decrement is:%d\n", dwMTUDec);

	ADAPTER_MODE Mode;
	Mode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL;
	Mode.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[iIndex];

	// Create notification event
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Set event for helper driver
	if ((!hEvent)||(!api.SetPacketEvent((HANDLE)AdList.m_nAdapterHandle[iIndex], hEvent)))
	{
		printf ("Failed to create notification event or set it for driver.\n");
		return 0;
	}

	atexit (ReleaseInterface);
	
	// Initialize Request
	barbaApp->CurrentRequest.hAdapterHandle = (HANDLE)AdList.m_nAdapterHandle[iIndex];
	api.SetAdapterMode(&Mode);

	bool terminate = false;
	while (!terminate)
	{
		WaitForSingleObject ( hEvent, INFINITE );
		while(api.ReadPacket(&barbaApp->CurrentRequest))
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
		ResetEvent(hEvent);
	}

	printf ("Filtering complete\n");
	return 0;
}

