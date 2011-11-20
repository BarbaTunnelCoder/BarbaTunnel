#pragma once

//BarbaServerConnection
class BarbaServerConnection
{
public:
	BarbaServerConnection(void)
	{
		ClientLocalIp = 0;
		ClientFakeIp = 0;
		ClientIp = 0;
		ClientPort = 0;
		ClientTunnelPort = 0;
		LastNegoTime = 0;
		ConfigItem = NULL;
		memset(ClientEthAddress, 0, _countof(ClientEthAddress));
	}


	DWORD ClientLocalIp;
	DWORD ClientFakeIp;
	DWORD ClientIp;
	u_short ClientPort;
	u_short ClientTunnelPort;
	BYTE ClientEthAddress[ETH_ALEN]; //Ethernet address of received packet; useful when router not exists
	DWORD LastNegoTime;
	BarbaServerConfigItem* ConfigItem;
	bool ProcessPacket(INTERMEDIATE_BUFFER* packet);

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer);
	bool ProcessPacketRedirect(INTERMEDIATE_BUFFER* packetBuffer);
	bool ProcessPacketUdpTunnel(INTERMEDIATE_BUFFER* packetBuffer);

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

	BarbaServerConnection* Find(DWORD clientIp, u_short clientPort, BarbaServerConfigItem* configItem)
	{
		for (size_t i=0; i<ConnectionsCount; i++)
		{
			if (Connections[i]->ClientIp==clientIp
				&& (clientPort==0 || Connections[i]->ClientPort==clientPort)
				&& (configItem==NULL || Connections[i]->ConfigItem==configItem))
				return Connections[i];
		}
		return NULL;
	}

	BarbaServerConnection* CreateConnection(PacketHelper* packet, BarbaServerConfigItem* configItem);
	void RemoveConnection(BarbaServerConnection* conn);
};


