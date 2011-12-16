#include "StdAfx.h"
#include "BarbaConnection.h"
#include "BarbaCrypt.h"
#include "BarbaApp.h"


BarbaConnection::BarbaConnection(LPCTSTR  connectionName, BarbaKey* key)
{
	this->LasNegotiationTime = GetTickCount();
	memcpy_s(&this->Key, sizeof BarbaKey, key, sizeof BarbaKey);
	_tcscpy_s(this->ConnectionName, connectionName);
}


BarbaConnection::~BarbaConnection(void)
{
}

LPCTSTR BarbaConnection::GetConnectionName()
{
	return ConnectionName;
}

DWORD BarbaConnection::GetLasNegotiationTime() 
{
	return LasNegotiationTime;
}

void BarbaConnection::CryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, this->Key.Key, this->Key.KeyCount);
}

void BarbaConnection::SetWorkingState(ULONG length, bool send)
{
	this->LasNegotiationTime = GetTickCount();
	theApp->Comm.SetWorkingState(length, send);
}
