#include "StdAfx.h"
#include "BarbaSocket.h"
#include "WinDivertFilterDriver.h"
#include "WinDivert\divert.h"


WinDivertFilterDriver::WinDivertFilterDriver(void)
{
	this->DivertHandle = NULL;
	this->MainIfIdx = 0;
	this->MainSubIfIdx = 0;
}


WinDivertFilterDriver::~WinDivertFilterDriver(void)
{
}

DWORD WinDivertFilterDriver::GetMTUDecrement()
{
	return 0;
}

void WinDivertFilterDriver::SetMTUDecrement(DWORD /*value*/)
{
	throw new BarbaException(_T("WinDivert does not support SetMTUDecrement!"));
}

void WinDivertFilterDriver::Initialize()
{
	//Nothing to initialize. it will do on start
}

void WinDivertFilterDriver::Dispose()
{
	//all resource disposed in Stop
}

void WinDivertFilterDriver::Stop()
{
	if (this->DivertHandle!=NULL)
		DivertClose(this->DivertHandle);
	this->DivertHandle = NULL;
	BarbaFilterDriver::Stop();
}

void WinDivertFilterDriver::StartCaptureLoop()
{
	 //&& tcp.SrcPort==65535
	this->DivertHandle = DivertOpen("ip.DstAddr>=74.125.225.0 && ip.DstAddr<74.125.227.0"); //mad
	if (this->DivertHandle==INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_INVALID_PARAMETER)
			throw new BarbaException(_T("WinDivert: filter syntax error!"));
		throw new BarbaException(_T("Failed to open Divert device (%d)!"), GetLastError());
	}

	//SendRouteFinderPacket
	SendRouteFinderPacket();

    // Main capture-modify-inject loop:
    DIVERT_ADDRESS addr;    // Packet address
    BYTE* buffer = new BYTE[0xFFFF];    // Packet buffer
    UINT recvLen;
	while (this->DivertHandle!=NULL && !StopEvent.IsSet())
    {
		//it will return error if this->DivertHandle closed
        if (!DivertRecv(this->DivertHandle, buffer, 0xFFFF, &addr, &recvLen))
            continue;

		printf("got\n");
		//Initialize Packet
		bool send = addr.Direction == DIVERT_PACKET_DIRECTION_OUTBOUND;
		PacketHelper* packet = new PacketHelper((iphdr_ptr)buffer);

		//user adapter index for any grab packet
		this->MainIfIdx = addr.IfIdx;
		this->MainSubIfIdx = addr.SubIfIdx;

		//add packet to queue
		AddPacket(packet, send);
    }

	delete buffer;
}

bool WinDivertFilterDriver::SendPacketToOutbound(PacketHelper* packet)
{
    DIVERT_ADDRESS addr;
	addr.IfIdx = this->MainIfIdx;
	addr.SubIfIdx = this->MainSubIfIdx;
	addr.Direction = DIVERT_PACKET_DIRECTION_OUTBOUND;
	return 
		this->DivertHandle!=NULL && 
		DivertSend(this->DivertHandle, packet->ipHeader, (UINT)packet->GetIpLen(), &addr, NULL)!=FALSE;
}

bool WinDivertFilterDriver::SendPacketToInbound(PacketHelper* packet)
{
	DIVERT_ADDRESS addr;
	addr.IfIdx = this->MainIfIdx;
	addr.SubIfIdx = this->MainSubIfIdx;
	addr.Direction = DIVERT_PACKET_DIRECTION_INBOUND;
	return 
		this->DivertHandle!=NULL && 
		DivertSend(this->DivertHandle, packet->ipHeader, (UINT)packet->GetIpLen(), &addr, NULL)!=FALSE;
}
