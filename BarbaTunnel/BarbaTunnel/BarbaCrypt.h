#pragma once
#include "PacketHelper.h"

class BarbaCrypt
{
public:
	static void Crypt(std::vector<BYTE>* buffer, std::vector<BYTE>* key, bool encrypt);
	static void Crypt(BYTE* buffer, size_t bufferCount, BYTE* key, size_t keyCount, bool encrypt);
	static void CryptPacket(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt);
	static void CryptUdp(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt);
	static void CryptTcp(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt);
};

