#pragma once
#include "BarbaServerConnection.h"

/*
 * This class is deprecated and UDP tunnel implemented by BarbaClientUdpConnection. 
 * It very sample and hello word fo whom that wants to add a protocol to BarbaTunnel
 * See how much simple is to add a tunnel protocol with BarbaTunnel!
 */
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
	BYTE ClientRouteEthAddress[ETH_ALEN]; //Ethernet address of received packet; useful when there is no router between client and server
};

