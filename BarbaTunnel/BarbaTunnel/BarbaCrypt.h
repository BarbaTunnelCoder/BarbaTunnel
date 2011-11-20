#pragma once
#include "PacketHelper.h"

class BarbaCrypt
{
public:
	static void Crypt(BYTE* buffer, int len);
	static void CryptPacket(PacketHelper* packet);
	static void CryptUdp(PacketHelper* packet);
	static void CryptTcp(PacketHelper* packet);
};

