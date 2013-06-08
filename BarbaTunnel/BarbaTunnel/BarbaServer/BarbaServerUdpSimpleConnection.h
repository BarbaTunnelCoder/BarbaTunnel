#pragma once
#include "BarbaServerConnection.h"

class BarbaServerUdpSimpleConnection : public BarbaServerConnection
{
public:
	explicit BarbaServerUdpSimpleConnection(BarbaServerConfig* config, u_long clientVirtualIp, PacketHelper* initPacket);
	virtual ~BarbaServerUdpSimpleConnection(void);
	bool ProcessOutboundPacket(PacketHelper* packet) override;
	bool ProcessInboundPacket(PacketHelper* packet) override;
	u_long GetSessionId() override;

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, PacketHelper* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, PacketHelper* orgPacketBuffer);
	u_long ClientLocalIp;
	u_short ClientPort;
	u_short TunnelPort;
	BYTE ClientRouteEthAddress[ETH_ALEN]; //Ethernet address of received packet; useful when router not exists

};

