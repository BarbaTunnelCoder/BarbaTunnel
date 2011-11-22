#pragma once
#include "PacketHelper.h"

class BarbaCrypt
{
public:
	static void Crypt(BYTE* buffer, size_t bufferCount, BYTE* key, size_t keyCount);
	static void CryptPacket(PacketHelper* packet, BYTE* key, size_t keyCount);
	static void CryptUdp(PacketHelper* packet, BYTE* key, size_t keyCount);
	static void CryptTcp(PacketHelper* packet, BYTE* key, size_t keyCount);
};

