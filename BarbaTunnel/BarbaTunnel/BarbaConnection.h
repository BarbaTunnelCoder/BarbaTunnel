#pragma once
#include "BarbaConfig.h"

class BarbaConnection
{
public:
	BarbaConnection()
	{
		PeerLocalIp = 0;
		PeerFakeIp = 0;
		PeerIP = 0;
		PeerPort = 0;
		LastNegoTime = 0;
		Sequence = 0;
		Config = NULL;
		act = false;
	}

	BarbaConfig* Config;
	BYTE PeerEthAddress[ETH_ALEN]; //Ethernet address of received packet; useful when router not exists
	DWORD PeerLocalIp;
	DWORD PeerFakeIp;
	DWORD PeerIP;
	u_short PeerPort; 
	DWORD LastNegoTime;
	DWORD Sequence;
	bool ProcessPacket(INTERMEDIATE_BUFFER* packet);

private:
	ether_header_ptr CreateBarbaPacket2(ether_header_ptr ethHeader);
	ether_header_ptr CreateBarbaPacket(INTERMEDIATE_BUFFER* packet);
	ether_header_ptr ExtractOrigianlPacketFromBarbaPacket(ether_header_ptr ethHeader);
	size_t CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer);
	bool act;
	void InitFakeConnection(ether_header_ptr ethHeader);

};

class BarbaConnectionManager
{
private:
	BarbaConnection* Connections[MAX_BARBA_CONNECTIONS];
	int ConnectionsCount;

public:
	BarbaConnectionManager()
	{
	}

	BarbaConnection* FindByIp(DWORD ip, u_short port)
	{
		for (int i=0; i<ConnectionsCount; i++)
			if (Connections[i]->PeerIP==ip && (port==0 || Connections[i]->PeerPort==port ) )
				return Connections[i];
		return NULL;
	}

	BarbaConnection* FindByFakeIp(DWORD ip)
	{
		for (int i=0; i<ConnectionsCount; i++)
			if (Connections[i]->PeerFakeIp==ip )
				return Connections[i];
		return NULL;
	}

	void RemoveConnection(BarbaConnection* conn)
	{
		for (int i=0; i<ConnectionsCount; i++)
		{
			if (Connections[i]==conn)
			{
				Connections[i]=Connections[ConnectionsCount-1];
				delete conn;
				ConnectionsCount--;
				break;
			}
		}
	}

	BarbaConnection* CreateConnection(DWORD ip, u_short port, BarbaConfig* config)
	{
		BarbaConnection* conn = new BarbaConnection();
		conn->PeerIP = ip;
		conn->PeerPort = port;
		conn->Config = config;
		Connections[ConnectionsCount++] = conn;
		return conn;
	}
};
