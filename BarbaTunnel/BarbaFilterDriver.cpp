#include "StdAfx.h"
#include "BarbaServer\BarbaServerApp.h"
#include "BarbaClient\BarbaClientApp.h"
#include "BarbaFilterDriver.h"

BarbaFilterDriver::BarbaFilterDriver(size_t maxCaptureMessageQueue)
	: StopEvent(true, false)
	, SocketHelper(AF_INET, SOCK_RAW, IPPROTO_RAW)
	, CaptureEvent(true, true)
{
	MaxCaptureMessageQueue = maxCaptureMessageQueue;
	_IsStarted = false;
	CaptureThreadHandle = NULL;
}

size_t BarbaFilterDriver::GetMaxPacketLen()
{
	return GetNetworkLayer()==NetworkLayerDateLink 
		? MAX_ETHER_FRAME
		: sizeof ether_header + 0xFFFF;
}

void BarbaFilterDriver::SendPacketWithSocket(PacketHelper* packet)
{
	SocketHelper.SendTo( packet->GetDesIp(), (BYTE*)packet->ipHeader, packet->GetIpLen() );
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
	_IsStarted = true;
	if (CaptureThreadHandle!=NULL)
		CloseHandle(CaptureThreadHandle); //delete old handle if start again without dispose
	CaptureThreadHandle = (HANDLE)_beginthreadex(NULL, 16000, CaptureThread, this, 0, NULL);
	ProcessCapturedPackets();
	_IsStarted = false;
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

	if (CaptureThreadHandle!=NULL)
	{
		WaitForSingleObject(CaptureThreadHandle, INFINITE);
		CloseHandle( CaptureThreadHandle );
	}
}

void BarbaFilterDriver::AddPacket(PacketHelper* packet, bool outbound)
{
	if (!packet->IsValidChecksum() )
	{
		BarbaLog2("Error: Invalid packet received and dropped!");
		delete packet;
		return;
	}

	//wait till pulse set
	SimpleLock lock(CapturePackets.GetCriticalSection());
	if (CapturePackets.GetCount()==MaxCaptureMessageQueue)
	{
		BarbaLog(_T("FilterDriver: Capture Message Queue is full. Packet dropped!"));
		delete packet;
		return;
	}


	CapturePackets.AddTail(new CapturePacket(packet, outbound));
	CaptureEvent.Set();
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
	events[0] = CaptureEvent.GetHandle();
	events[1] = StopEvent.GetHandle();
	while (!StopEvent.IsSet())
	{
		WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
		CapturePacket* capturePacket = CapturePackets.RemoveHead();
		try
		{
			while (capturePacket!=NULL)
			{
				PacketHelper packet(capturePacket->Packet); //keep copy to prevent change
				if (!theApp->ProcessFilterDriverPacket(&packet, capturePacket->Outbound))
				{
					if (capturePacket->Outbound)
						SendPacketToOutbound(capturePacket->Packet);
					else
						SendPacketToInbound(capturePacket->Packet);
				}
				
				delete capturePacket;
				capturePacket = CapturePackets.RemoveHead();
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

void BarbaFilterDriver::AddClientFilters(void* filter, BarbaArray<BarbaClientConfig>* configs)
{
	for (size_t i=0; i<configs->size(); i++)
	{
		BarbaClientConfig* config = &configs->at(i);
		if (!config->Enabled || config->ServerIp==0)
			continue;

		//filter only required packet going to server
		for (size_t j=0; j<config->GrabProtocols.size(); j++)
			AddFilter(filter, true, 0, 0, config->ServerIp, 0, config->GrabProtocols[j].Protocol, 0, 0, config->GrabProtocols[j].Port, 0);

		//redirect port
		if (config->RealPort!=0)
			AddFilter(filter, true, 0, 0, config->ServerIp, 0, config->GetTunnelProtocol(), 0, 0, config->RealPort, 0);

		//filter only tunnel packet that come from server except tcp-tunnel family that use socket
		if (config->Mode!=BarbaModeTcpTunnel && config->Mode!=BarbaModeHttpTunnel) 
			for (size_t k=0; k<config->TunnelPorts.Items.size(); k++)
				AddFilter(filter, false, config->ServerIp, 0, 0, 0, config->GetTunnelProtocol(), config->TunnelPorts.Items[k].StartPort, config->TunnelPorts.Items[k].EndPort, 0, 0);
	}
}

void BarbaFilterDriver::AddServerFilters(void* filter, BarbaArray<BarbaServerConfig>* configs)
{
	//filter incoming tunnel
	for (size_t i=0; i<configs->size(); i++)
	{
		BarbaServerConfig* config = &configs->at(i);
		if (!config->Enabled)
			continue;

		//filter only tunnel packet that come from server except http-tunnel that use socket
		if (config->Mode!=BarbaModeTcpTunnel && config->Mode!=BarbaModeHttpTunnel) 
			for (size_t j=0; j<config->TunnelPorts.Items.size(); j++)
				AddFilter(filter, false, 0, 0, config->ServerIp, 0, config->GetTunnelProtocol(), 0, 0, config->TunnelPorts.Items[j].StartPort, config->TunnelPorts.Items[j].EndPort);

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
}
