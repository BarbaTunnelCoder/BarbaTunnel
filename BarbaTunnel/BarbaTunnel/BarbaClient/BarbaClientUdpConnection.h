#pragma once
#include "BarbaClientConnection.h"

class BarbaClientUdpConnection :
	public BarbaClientConnection
{
public:
	explicit BarbaClientUdpConnection(BarbaClientConfig* config, u_short clientPort, u_short tunnelPort);
	virtual ~BarbaClientUdpConnection(void);
	virtual bool ProcessPacket(PacketHelper* packet, bool send);
	virtual bool ShouldProcessPacket(PacketHelper* packet);
	virtual u_short GetTunnelPort();

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, PacketHelper* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, PacketHelper* orgPacket);
	u_short ClientPort;
	u_short TunnelPort;

};

