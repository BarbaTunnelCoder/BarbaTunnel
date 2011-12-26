#include "StdAfx.h"
#include "BarbaConnection.h"
#include "BarbaCrypt.h"
#include "BarbaApp.h"


int LastConnectionId = 0;
BarbaConnection::BarbaConnection()
{
	this->ConnectionId = ++LastConnectionId;
	this->LasNegotiationTime = GetTickCount();
}


BarbaConnection::~BarbaConnection(void)
{
}

u_int BarbaConnection::GetLasNegotiationTime() 
{
	return this->LasNegotiationTime;
}

void BarbaConnection::EncryptData(BYTE* data, size_t dataLen)
{
	BarbaCrypt::Crypt(data, dataLen, GetKey()->data(), GetKey()->size(), true);
}

void BarbaConnection::DecryptData(BYTE* data, size_t dataLen)
{
	BarbaCrypt::Crypt(data, dataLen, GetKey()->data(), GetKey()->size(), false);
}

void BarbaConnection::EncryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, GetKey()->data(), GetKey()->size(), true);
}

void BarbaConnection::DecryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, GetKey()->data(), GetKey()->size(), false);
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
