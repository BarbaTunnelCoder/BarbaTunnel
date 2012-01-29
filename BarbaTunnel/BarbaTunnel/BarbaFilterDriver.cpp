#include "StdAfx.h"
#include "BarbaFilterDriver.h"
#include "BarbaApp.h"

#define RouteFinderIp "173.194.34.48"

BarbaFilterDriver::BarbaFilterDriver(size_t maxCaptureMessageQueue)
	: StopEvent(true, false)
	, RouteFinderSocket(AF_INET, SOCK_RAW, IPPROTO_NONE)
	, RouteFinderPacket(IPPROTO_IP, sizeof iphdr)
	, CaptureEvent(true, true)
{
	this->MaxCaptureMessageQueue = maxCaptureMessageQueue;
	this->_IsStarted = false;
	this->CaptureThreadHandle = NULL;
}

size_t BarbaFilterDriver::GetMaxPacketLen()
{
	return GetNetworkLayer()==NetworkLayerDateLink 
		? sizeof MAX_ETHER_FRAME 
		: sizeof ether_header + 0xFFFF;
}

void BarbaFilterDriver::SendRouteFinderPacket()
{
	RouteFinderSocket.SendTo( GetRouteFinderPacket()->GetDesIp(), (BYTE*)GetRouteFinderPacket()->ipHeader, GetRouteFinderPacket()->GetIpLen() );
}

PacketHelper* BarbaFilterDriver::GetRouteFinderPacket()
{ 
	//initialize RouteFinderPacket
	if (this->RouteFinderPacket.GetDesPort() == 0)
	{
		this->RouteFinderPacket.SetSrcIp( inet_addr("127.0.0.1") );
		this->RouteFinderPacket.SetDesIp( inet_addr(RouteFinderIp) );
		this->RouteFinderPacket.RecalculateChecksum();
	}
	return &this->RouteFinderPacket; 
}

bool BarbaFilterDriver::IsRouteFinderPacket( PacketHelper* packet )
{
	return 
		GetRouteFinderPacket()->ipHeader->ip_p == packet->ipHeader->ip_p &&
		GetRouteFinderPacket()->GetSrcIp() == packet->GetSrcIp() &&
		GetRouteFinderPacket()->GetDesIp() == packet->GetDesIp() &&
		GetRouteFinderPacket()->GetSrcPort() == packet->GetSrcPort() &&
		GetRouteFinderPacket()->GetDesPort() == packet->GetDesPort();
}

BarbaFilterDriver::~BarbaFilterDriver(void)
{
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
		return;

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
	_this->StartCaptureLoop();
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


