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

size_t BarbaConnection::GetLasNegotiationTime() 
{
	return this->LasNegotiationTime;
}

void BarbaConnection::CryptData(BYTE* data, size_t dataSize, size_t index, bool encrypt)
{
	BarbaCrypt::Crypt(data, dataSize, GetKey()->data(), GetKey()->size(), index, encrypt);
}

void BarbaConnection::EncryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, GetKey()->data(), GetKey()->size(), true);
}

void BarbaConnection::DecryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, GetKey()->data(), GetKey()->size(), false);
}

void BarbaConnection::SetWorkingState(size_t length, bool send)
{
	this->LasNegotiationTime = GetTickCount();
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
