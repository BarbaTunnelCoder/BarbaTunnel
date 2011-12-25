#include "StdAfx.h"
#include "BarbaCrypt.h"
#include "BarbaUtils.h"

void BarbaCrypt::Crypt(BYTE* buffer, size_t bufferCount, BYTE* key, size_t keyCount, bool /*encrypt*/)
{
	if (keyCount==0)
		return;

	for (size_t i=0; i<bufferCount; i++)
	{
		buffer[i] ^= key[i%keyCount];
	}
}

void BarbaCrypt::CryptPacket(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt)
{
	if (packet->IsTcp())
		CryptTcp(packet, key, keyCount, encrypt);
	if (packet->IsUdp())
		CryptUdp(packet, key, keyCount, encrypt);
}

void BarbaCrypt::CryptUdp(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt)
{
	Crypt(packet->GetUdpPayload(), packet->GetUdpPayloadLen(), key, keyCount, encrypt);
	Crypt(packet->GetIpExtraHeader(), packet->GetIpExtraHeaderLen(), key, keyCount, encrypt);
}

void BarbaCrypt::CryptTcp(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt)
{
	Crypt(packet->GetTcpExtraHeader(), packet->GetTcpExtraHeaderLen() + packet->GetTcpPayloadLen(), key, keyCount, encrypt);
	Crypt(packet->GetIpExtraHeader(), packet->GetIpExtraHeaderLen(), key, keyCount, encrypt);
}
