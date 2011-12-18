#pragma once
#include "BarbaServerConnection.h"
#include "BarbaServerHttpCourier.h"

class BarbaServerHttpConnection :
	public BarbaServerConnection
{
public:
	explicit BarbaServerHttpConnection(BarbaServerConfigItem* configItem, 
		u_long clientVirtualIp, u_long clientIp, u_short tunnelPort, u_long sessionId);
	virtual ~BarbaServerHttpConnection(void);
	virtual bool ShouldProcessPacket(PacketHelper* packet);
	virtual bool ProcessPacket(PacketHelper* packet, bool send);
	virtual u_short GetTunnelPort() {return this->TunnelPort;}
	virtual u_long GetSessionId() {return this->SessionId;}
	bool AddSocket(BarbaSocket* Socket, bool isOutgoing);

private:
	BarbaServerHttpCourier* Courier;
	u_long SessionId;
	u_short TunnelPort;
};

