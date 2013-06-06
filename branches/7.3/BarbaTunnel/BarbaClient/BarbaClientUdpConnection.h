#pragma once
#include "BarbaClientConnection.h"

class BarbaClientUdpConnection :
	public BarbaClientConnection
{
public:
	explicit BarbaClientUdpConnection(BarbaClientConfig* config, u_short clientPort, u_short tunnelPort);
	virtual ~BarbaClientUdpConnection(void);
	bool ProcessPacket(PacketHelper* packet, bool send) override;
	bool ShouldProcessPacket(PacketHelper* packet) override;
	virtual u_short GetTunnelPort() override;

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, PacketHelper* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, PacketHelper* orgPacket);
	u_short ClientPort;
	u_short TunnelPort;

};

