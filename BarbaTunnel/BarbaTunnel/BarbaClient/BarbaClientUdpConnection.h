#pragma once
#include "BarbaClientConnection.h"

class BarbaClientUdpConnection :
	public BarbaClientConnection
{
public:
	explicit BarbaClientUdpConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem, u_short clientPort, u_short tunnelPort);
	virtual ~BarbaClientUdpConnection(void);
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer);
	virtual bool ShouldProcessPacket(PacketHelper* packet);
	virtual u_short GetTunnelPort();

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer);
	u_short ClientPort;
	u_short TunnelPort;

};

