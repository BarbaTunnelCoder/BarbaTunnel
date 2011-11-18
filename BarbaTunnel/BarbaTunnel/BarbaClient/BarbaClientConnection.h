#pragma once
#include "BarbaClientConfig.h"

//BarbaClientConnection
class BarbaClientConnection
{
public:
	BarbaClientConnection();
	BarbaClientConfig* Config;
	BarbaClientConfigItem* ConfigItem;
	DWORD LasNegotiationTime;
	u_short ClientPort;
	bool ProcessPacket(INTERMEDIATE_BUFFER* packet);

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer);

};

//BarbaClientConnectionManager
class BarbaClientConnectionManager
{
private:
	BarbaClientConnection* Connections[MAX_BARBA_CONNECTIONS];
	size_t ConnectionsCount;

public:
	BarbaClientConnectionManager()
	{
		ConnectionsCount = 0;
	}

	BarbaClientConnection* Find(DWORD serverIp, u_char tunnelProtocol, u_short clientPort)
	{
		for (size_t i=0; i<ConnectionsCount; i++)
		{
			if (Connections[i]->Config->ServerIp==serverIp 
				&& (tunnelProtocol==0 || BarbaMode_GetProtocol(Connections[i]->ConfigItem->Mode)==tunnelProtocol)
				&& (clientPort==0 || Connections[i]->ConfigItem->TunnelPort==clientPort))
				return Connections[i];
		}
		return NULL;
	}

	BarbaClientConnection* FindByConfigItem(BarbaClientConfigItem* configItem)
	{
		for (size_t i=0; i<ConnectionsCount; i++)
		{
			if (Connections[i]->ConfigItem==configItem)
				return Connections[i];
		}
		return NULL;
	}

	void RemoveConnection(BarbaClientConnection* conn)
	{
		for (size_t i=0; i<ConnectionsCount; i++)
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

	BarbaClientConnection* CreateConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem)
	{
		BarbaClientConnection* conn = new BarbaClientConnection();
		conn->Config = config;
		conn->ConfigItem = configItem;
		conn->ClientPort = configItem->TunnelPort; //currently same as tunnel protocol
		Connections[ConnectionsCount++] = conn;
		return conn;
	}
};
