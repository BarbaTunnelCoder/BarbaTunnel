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
