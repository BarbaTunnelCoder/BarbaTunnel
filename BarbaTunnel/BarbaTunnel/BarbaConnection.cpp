#include "StdAfx.h"
#include "BarbaConnection.h"
#include "BarbaCrypt.h"
#include "BarbaApp.h"


BarbaConnection::BarbaConnection()
{
	this->LasNegotiationTime = GetTickCount();
}


BarbaConnection::~BarbaConnection(void)
{
}

DWORD BarbaConnection::GetLasNegotiationTime() 
{
	return LasNegotiationTime;
}

void BarbaConnection::CryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, GetKey()->Key, GetKey()->KeyCount);
}

void BarbaConnection::SetWorkingState(ULONG length, bool send)
{
	this->LasNegotiationTime = GetTickCount();
	theApp->Comm.SetWorkingState(length, send);
}

bool BarbaConnection::SendPacketToAdapter(PacketHelper* packet)
{
	SetWorkingState(packet->GetIpLen(), true);
	packet->RecalculateChecksum();
	return theApp->SendPacketToAdapter(packet);
}

bool BarbaConnection::SendPacketToMstcp(PacketHelper* packet)
{
	SetWorkingState(packet->GetIpLen(), false);
	packet->RecalculateChecksum();
	return theApp->SendPacketToMstcp(packet);
}
