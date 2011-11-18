#pragma once

//BarbaServerConnection
class BarbaServerConnection
{
public:
	BarbaServerConnection(void);
	~BarbaServerConnection(void);
	bool ProcessPacket(INTERMEDIATE_BUFFER* packet);

	DWORD ClientLocalIp;
	DWORD ClientFakeIp;
	DWORD ClientIp;
	BYTE ClientEthAddress[ETH_ALEN]; //Ethernet address of received packet; useful when router not exists
	DWORD LastNegoTime;

private:
	size_t CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer);

};

//BarbaServerConnectionManager
class BarbaServerConnectionManager
{
private:
	BarbaServerConnection* Connections[MAX_BARBA_CONNECTIONS];
	size_t ConnectionsCount;

public:
	BarbaServerConnectionManager()
	{
		ConnectionsCount = 0;
	}

	BarbaServerConnection* FindByFakeIp(DWORD ip)
	{
		for (size_t i=0; i<ConnectionsCount; i++)
			if (Connections[i]->ClientFakeIp==ip )
				return Connections[i];
		return NULL;
	}

};


