#include "StdAfx.h"
#include "BarbaConnection.h"
#include "BarbaCrypt.h"
#include "BarbaApp.h"


int LastConnectionId = 0;
BarbaConnection::BarbaConnection(BarbaConfig* config)
{
	_Config = config;
	ConnectionId = ++LastConnectionId;
	LasNegotiationTime = GetTickCount();
}

void BarbaConnection::LogImpl(int level, LPCTSTR format, va_list _ArgList)
{
	TCHAR* msg = new TCHAR[1000];
	_vstprintf_s(msg, 1000, format, _ArgList);
	
	TCHAR* msg2 = new TCHAR[1000];
	_stprintf_s(msg2, 1000, _T("BarbaConnection: Name: %s, SessionId: %x, %s"), GetConfig()->GetName(theApp->LogAnonymously).data(), GetSessionId(),  msg);

	va_list emptyList = {0};
	BarbaLogImpl(level, msg2, emptyList);
	delete msg;
	delete msg2;
}

void BarbaConnection::Log2(LPCTSTR format, ...) { va_list argp; va_start(argp, format); LogImpl(2, format, argp); va_end(argp); }
void BarbaConnection::Log3(LPCTSTR format, ...) { va_list argp; va_start(argp, format); LogImpl(3, format, argp); va_end(argp); }

BarbaConnection::~BarbaConnection(void)
{
}

DWORD BarbaConnection::GetLasNegotiationTime() 
{
	return LasNegotiationTime;
}

void BarbaConnection::Init()
{
}

void BarbaConnection::CryptData(BYTE* data, size_t dataSize, size_t index, bool encrypt)
{
	BarbaCrypt::Crypt(data, dataSize, GetConfig()->Key.data(), GetConfig()->Key.size(), index, encrypt);
}

void BarbaConnection::EncryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, GetConfig()->Key.data(), GetConfig()->Key.size(), true);
}

void BarbaConnection::DecryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, GetConfig()->Key.data(), GetConfig()->Key.size(), false);
}

void BarbaConnection::SetWorkingState(size_t length, bool send)
{
	LasNegotiationTime = GetTickCount();
	theApp->Comm.SetWorkingState(length, send);
}

bool BarbaConnection::SendPacketToOutbound(PacketHelper* packet)
{
	SetWorkingState(packet->GetIpLen(), true);
	packet->RecalculateChecksum();
	return theApp->SendPacketToOutbound(packet);
}

bool BarbaConnection::SendPacketToInbound(PacketHelper* packet)
{
	SetWorkingState(packet->GetIpLen(), false);
	packet->RecalculateChecksum();
	return theApp->SendPacketToInbound(packet);
}
