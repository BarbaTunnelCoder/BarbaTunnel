#include "StdAfx.h"
#include "BarbaCrypt.h"
#include "BarbaUtils.h"

void BarbaCrypt::Crypt(BYTE* buffer, int len)
{
	for (int i=0; i<len; i++)
	{
		buffer[i] ^= i+1;
	}
}

void BarbaCrypt::CryptPacket(PacketHelper* packet)
{
	if (packet->IsTcp())
		CryptTcp(packet);
	if (packet->IsUdp())
		CryptUdp(packet);
}

void BarbaCrypt::CryptUdp(PacketHelper* packet)
{
	Crypt(packet->GetUdpPayload(), packet->GetUdpPayloadLen());
	Crypt(packet->GetIpExtraHeader(), packet->GetIpExtraHeaderLen());
}

void BarbaCrypt::CryptTcp(PacketHelper* packet)
{
	Crypt(packet->GetTcpExtraHeader(), packet->GetTcpExtraHeaderLen() + packet->GetTcpPayloadLen());
	Crypt(packet->GetIpExtraHeader(), packet->GetIpExtraHeaderLen());
}
