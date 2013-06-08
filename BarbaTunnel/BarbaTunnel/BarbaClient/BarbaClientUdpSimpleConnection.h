#pragma once
#include "BarbaClientConnection.h"

class BarbaClientUdpSimpleConnection : 	public BarbaClientConnection
{
public:
	explicit BarbaClientUdpSimpleConnection(BarbaClientConfig* config, PacketHelper* initPacket);
	virtual ~BarbaClientUdpSimpleConnection(void);
	bool ProcessOutboundPacket(PacketHelper * packet) override;
	bool ProcessInboundPacket(PacketHelper * packet) override;
	u_long GetSessionId() override;

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, PacketHelper* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, PacketHelper* orgPacket);
	u_short ClientPort;
	u_short TunnelPort;

};

