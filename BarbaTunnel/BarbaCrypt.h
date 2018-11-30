#pragma once
class BarbaCrypt
{
public:
	static void Crypt(BarbaBuffer* buffer, BarbaBuffer* key, size_t index, bool encrypt);
	static void Crypt(BYTE* buffer, size_t bufferCount, BYTE* key, size_t keyCount, size_t index, bool encrypt);
	static void CryptPacket(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt);
	static void CryptUdp(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt);
	static void CryptTcp(PacketHelper* packet, BYTE* key, size_t keyCount, bool encrypt);
};

