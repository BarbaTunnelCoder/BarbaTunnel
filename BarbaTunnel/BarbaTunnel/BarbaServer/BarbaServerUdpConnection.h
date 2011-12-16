#pragma once
#include "BarbaServerConnection.h"

class BarbaServerUdpConnection : public BarbaServerConnection
{
public:
	explicit BarbaServerUdpConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, 
		u_long clientIp, u_short clientPort, u_short tunnelPort, BYTE* clientRouteEthAddress);
	virtual ~BarbaServerUdpConnection(void);
	virtual bool ShouldProcessPacket(PacketHelper* packet);
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packet);
	virtual u_short GetTunnelPort();

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer);
	u_long ClientLocalIp;
	u_short ClientPort;
	u_short TunnelPort;
	BYTE ClientRouteEthAddress[ETH_ALEN]; //Ethernet address of received packet; useful when router not exists

};

