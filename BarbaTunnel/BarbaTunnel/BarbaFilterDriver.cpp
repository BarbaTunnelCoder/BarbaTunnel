#include "StdAfx.h"
#include "BarbaServer\BarbaServerApp.h"
#include "BarbaClient\BarbaClientApp.h"
#include "BarbaFilterDriver.h"

#define RouteFinderIp "173.194.34.48"

BarbaFilterDriver::BarbaFilterDriver(size_t maxCaptureMessageQueue)
	: StopEvent(true, false)
	, RouteFinderSocket(AF_INET, SOCK_RAW, IPPROTO_NONE)
	, RouteFinderPacket(IPPROTO_NONE, sizeof iphdr)
	, CaptureEvent(true, true)
{
	this->MaxCaptureMessageQueue = maxCaptureMessageQueue;
	this->_IsStarted = false;
	this->CaptureThreadHandle = NULL;

	//initialize RouteFinderPacket
	this->RouteFinderPacket.SetSrcIp( inet_addr("127.0.0.1") );
	this->RouteFinderPacket.SetDesIp( inet_addr(RouteFinderIp) );
	this->RouteFinderPacket.RecalculateChecksum();
}

size_t BarbaFilterDriver::GetMaxPacketLen()
{
	return GetNetworkLayer()==NetworkLayerDateLink 
		? MAX_ETHER_FRAME
		: sizeof ether_header + 0xFFFF;
}

void BarbaFilterDriver::SendRouteFinderPacket()
{
	PacketHelper* routeFinderPacket = &this->RouteFinderPacket;
	RouteFinderSocket.SendTo( routeFinderPacket->GetDesIp(), (BYTE*)routeFinderPacket->ipHeader, routeFinderPacket->GetIpLen() );
}

bool BarbaFilterDriver::IsRouteFinderPacket( PacketHelper* packet )
{
	PacketHelper* routeFinderPacket = &this->RouteFinderPacket;
	return 
		routeFinderPacket->ipHeader->ip_p == packet->ipHeader->ip_p &&
		routeFinderPacket->GetDesIp() == packet->GetDesIp() &&
		routeFinderPacket->GetDesPort() == packet->GetDesPort();
}

BarbaFilterDriver::~BarbaFilterDriver(void)
{
}

void BarbaFilterDriver::UpdateMTUDecrement()
{
	if (theApp->GetMTUDecrement()==-1 || theApp->GetMTUDecrement() == (int)GetMTUDecrement()  )
		return;

	BarbaLog(_T("Trying to set new MTU decrement to %d."), theApp->GetMTUDecrement());
	SetMTUDecrement( theApp->GetMTUDecrement() ) ;
	if ((int)GetMTUDecrement()!=theApp->GetMTUDecrement())
		throw new BarbaException(_T("Could not set new MTU decrement!"));

	LPCTSTR msg = 
		_T("BarbaTunnel set new MTU decrement to have enough space for adding Barba header to your packet.\n\n")
		_T("You must restart Windows so the new MTU decrement can effect.");
	BarbaNotify(_T("Error: Restart Windows Required!\r\nYou should restart your Windows to set new MTU decrement!"));
	throw new BarbaException(msg);

	//	BarbaUtils::SimpleShellExecute(_T("shutdown.exe"), _T("/r /t 0 /d p:4:2"), SW_HIDE);
}

void BarbaFilterDriver::Start()
{
	this->_IsStarted = true;
	if (this->CaptureThreadHandle!=NULL)
		CloseHandle(this->CaptureThreadHandle); //delete old handle if start again without dispose
	this->CaptureThreadHandle = (HANDLE)_beginthreadex(NULL, 16000, CaptureThread, this, 0, NULL);
	ProcessCapturedPackets();
	this->_IsStarted = false;
}

void BarbaFilterDriver::Stop()
{
	StopEvent.Set();
	CaptureEvent.Set();
}

void BarbaFilterDriver::Dispose()
{
	if (IsStarted())
		Stop();

	if (this->CaptureThreadHandle!=NULL)
	{
		WaitForSingleObject(this->CaptureThreadHandle, INFINITE);
		CloseHandle( this->CaptureThreadHandle );
	}
}

void BarbaFilterDriver::AddPacket(PacketHelper* packet, bool send)
{
	//Don't process RouteFinderPacket
	if (IsRouteFinderPacket(packet))
	{
		delete packet;
		return;
	}

	//wait till pulse set
	SimpleLock lock(this->CapturePackets.GetCriticalSection());
	if (this->CapturePackets.GetCount()==this->MaxCaptureMessageQueue)
	{
		BarbaLog(_T("FilterDriver: Capture Message Queue is full. Packet dropped!"));
		delete packet;
		return;
	}


	this->CapturePackets.AddTail(new CapturePacket(packet, send));
	this->CaptureEvent.Set();
}

unsigned int __stdcall BarbaFilterDriver::CaptureThread(void* data)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	BarbaFilterDriver* _this = (BarbaFilterDriver*)data;
	try
	{
		_this->StartCaptureLoop();
	}
	catch (BarbaException* err)
	{
		BarbaLog(_T("%s FilterDriver: Error: %s"), _this->GetName(), err->ToString());
		delete err;
	}
	catch (TCHAR* err)
	{
		BarbaLog(_T("%s FilterDriver: Error: %s"), _this->GetName(), err);
	}
	catch(...)
	{
		BarbaLog(_T("%s KernelFilterDriver: Unknown error while processing packet!"), _this->GetName());
	}
	_this->StopEvent.Set();
	return 0;
}

void BarbaFilterDriver::ProcessCapturedPackets()
{
	HANDLE events[2];
	events[0] = this->CaptureEvent.GetHandle();
	events[1] = this->StopEvent.GetHandle();
	while (!this->StopEvent.IsSet())
	{
		WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
		CapturePacket* capturePacket = this->CapturePackets.RemoveHead();
		try
		{
			while (capturePacket!=NULL)
			{
				if (!theApp->ProcessFilterDriverPacket(capturePacket->Packet, capturePacket->Send))
				{
					if (capturePacket->Send)
						this->SendPacketToOutbound(capturePacket->Packet);
					else
						this->SendPacketToInbound(capturePacket->Packet);
				}
				
				delete capturePacket;
				capturePacket = this->CapturePackets.RemoveHead();
			}
		}
		catch (BarbaException* err)
		{
			BarbaLog(_T("FilterDriver: Error: %s"), err->ToString());
			delete err;
		}
		catch (TCHAR* err)
		{
			BarbaLog(_T("FilterDriver: Error: %s"), err);
		}
		catch(...)
		{
			BarbaLog(_T("FilterDriver: Unknown error while processing packet!"));
		}

		//delete capturePacket if throw an exception and capturePacket not deleted yet
		if (capturePacket!=NULL)
			delete capturePacket;

		//reset if there is no message
		SimpleLock lock(this->CapturePackets.GetCriticalSection());
		if (this->CapturePackets.IsEmpty())
			this->CaptureEvent.Reset();
		lock.Unlock();
	}

	//wait for Capture thread to finish
	WaitForSingleObject(this->CaptureThreadHandle, INFINITE);

	//delete all other unprocessed packets
	CapturePacket* capturePacket = this->CapturePackets.RemoveHead();
	while (capturePacket!=NULL)
	{
		delete capturePacket;
		capturePacket = this->CapturePackets.RemoveHead();
	}
}

void BarbaFilterDriver::AddClientFilters(void* filter, std::vector<BarbaClientConfig>* configs)
{
	for (size_t i=0; i<configs->size(); i++)
	{
		BarbaClientConfig* config = &configs->at(i);
		if (!config->Enabled)
			continue;

		//filter only required packet going to server
		for (size_t i=0; i<config->GrabProtocols.size(); i++)
			AddFilter(filter, true, 0, 0, config->ServerIp, 0, config->GrabProtocols[i].Protocol, 0, 0, config->GrabProtocols[i].Port, 0);

		//redirect port
		if (config->RealPort!=0)
			AddFilter(filter, true, 0, 0, config->ServerIp, 0, config->GetTunnelProtocol(), 0, 0, config->RealPort, 0);

		//filter only tunnel packet that come from server except http-tunnel that use socket
		if (config->Mode!=BarbaModeHttpTunnel) 
			for (size_t i=0; i<config->TunnelPorts.size(); i++)
				AddFilter(filter, false, config->ServerIp, 0, 0, 0, config->GetTunnelProtocol(), config->TunnelPorts[i].StartPort, config->TunnelPorts[i].EndPort, 0, 0);
	}
}

void BarbaFilterDriver::AddServerFilters(void* filter, std::vector<BarbaServerConfig>* configs)
{
	//filter incoming tunnel
	for (size_t i=0; i<configs->size(); i++)
	{
		BarbaServerConfig* config = &configs->at(i);
		if (!config->Enabled)
			continue;

		//filter only tunnel packet that come from server except http-tunnel that use socket
		if (config->Mode!=BarbaModeHttpTunnel) 
			for (size_t i=0; i<config->TunnelPorts.size(); i++)
				AddFilter(filter, false, 0, 0, config->ServerIp, 0, config->GetTunnelProtocol(), 0, 0, config->TunnelPorts[i].StartPort, config->TunnelPorts[i].EndPort);

		//filter ICMP for debug mode
		if (theApp->IsDebugMode())
			AddFilter(filter, false, 0, 0, config->ServerIp, 0, IPPROTO_ICMP, 0, 0, 0, 0);
	}

	//filter outgoing virtual IP
	AddFilter(filter, true, 0, 0, theServerApp->VirtualIpRange.StartIp, theServerApp->VirtualIpRange.EndIp, 0, 0, 0, 0, 0);
}

void BarbaFilterDriver::AddPacketFilter(void* filter)
{
	if (theApp->IsServerMode())
		AddServerFilters(filter, &theServerApp->Configs);
	else
		AddClientFilters(filter, &theClientApp->Configs);

	//add route finder filter
	PacketHelper* routeFinderPacket = &this->RouteFinderPacket;
	AddFilter(filter, true, 0, 0, routeFinderPacket->GetDesIp(), 0, routeFinderPacket->ipHeader->ip_p, routeFinderPacket->GetSrcPort(), 0, routeFinderPacket->GetDesPort(), 0);
}
