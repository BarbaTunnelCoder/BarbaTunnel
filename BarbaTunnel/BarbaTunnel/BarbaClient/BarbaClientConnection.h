#pragma once
#include "BarbaClientConfig.h"

//BarbaClientConnection
class BarbaClientConnection
{
public:
	BarbaClientConnection()
	{
		Config = NULL;
		ConfigItem = NULL;
		LasNegotiationTime = 0;
		ClientPort = 0;
	}

	BarbaClientConfig* Config;
	BarbaClientConfigItem* ConfigItem;
	DWORD LasNegotiationTime;
	u_short ClientPort;
	bool ProcessPacket(INTERMEDIATE_BUFFER* packet);
	void ReportConnection();

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer);
	bool ProcessPacketRedirect(INTERMEDIATE_BUFFER* packetBuffer);
	bool ProcessPacketUdpTunnel(INTERMEDIATE_BUFFER* packetBuffer);
	void CryptPacket(PacketHelper* packet);
};

//BarbaClientConnectionManager
class BarbaClientConnectionManager
{
private:
	BarbaClientConnection* Connections[BARBA_MAX_CONNECTIONS];
	size_t ConnectionsCount;

	void CleanTimeoutConnections()
	{
		for (size_t i=0; i<ConnectionsCount; i++)
		{
			if (GetTickCount()-Connections[i]->LasNegotiationTime>BARBA_CONNECTION_TIMEOUT)
			{
				RemoveConnection(Connections[i]);
				i--;
			}
		}
	}

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
				&& (tunnelProtocol==0 || tunnelProtocol==BarbaMode_GetProtocol(Connections[i]->ConfigItem->Mode))
				&& (clientPort==0 || clientPort==Connections[i]->ClientPort))
				return Connections[i];
		}
		return NULL;
	}

	BarbaClientConnection* FindByConfigItem(BarbaClientConfigItem* configItem, u_short clientPort)
	{
		for (size_t i=0; i<ConnectionsCount; i++)
		{
			if (Connections[i]->ConfigItem==configItem 
				&& (clientPort==0 || clientPort==Connections[i]->ClientPort))
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

	BarbaClientConnection* CreateConnection(PacketHelper* packet, BarbaClientConfig* config, BarbaClientConfigItem* configItem)
	{
		CleanTimeoutConnections();

		BarbaClientConnection* conn = new BarbaClientConnection();
		conn->Config = config;
		conn->ConfigItem = configItem;
		conn->ClientPort = packet->GetSrcPort();
		conn->ReportConnection();
		conn->LasNegotiationTime = GetTickCount();
		Connections[ConnectionsCount++] = conn;
		return conn;
	}
};
