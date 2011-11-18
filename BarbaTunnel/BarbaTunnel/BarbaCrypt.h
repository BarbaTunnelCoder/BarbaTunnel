#pragma once
#include "PacketHelper.h"

class BarbaCrypt
{
public:
	static void Crypt(BYTE* buffer, int len);
	static void CryptUdp(PacketHelper* packet);
	static void CryptTcp(PacketHelper* packet);
};

