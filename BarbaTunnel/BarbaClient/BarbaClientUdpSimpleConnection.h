#pragma once
#include "BarbaClientConnection.h"

/*
 * This class is deprecated and UDP tunnel implemented by BarbaClientUdpConnection. 
 * It very sample and hello word fo whom that wants to add a protocol to BarbaTunnel
 * See how much simple is to add a tunnel protocol with BarbaTunnel!
 */
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
	DWORD LocalIp;

};

