#pragma once
#include "PacketHelper.h"

class BarbaCrypt
{
public:
	static void Crypt(BYTE* buffer, size_t bufferCount, BYTE* key, size_t keyCount, bool encrypt=true);
	static void CryptPacket(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt=true);
	static void CryptUdp(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt=true);
	static void CryptTcp(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt=true);
};

