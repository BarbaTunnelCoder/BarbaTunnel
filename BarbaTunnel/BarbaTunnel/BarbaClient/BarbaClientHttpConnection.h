#pragma once
#include "BarbaClientConnection.h"
#include "BarbaClientHttpCourier.h"
class BarbaClientHttpConnection : public BarbaClientConnection
{
public:
	explicit BarbaClientHttpConnection(BarbaClientConfig* config, u_short tunnelPort);
	virtual ~BarbaClientHttpConnection(void);
	virtual bool ShouldProcessPacket(PacketHelper* packet);
	virtual bool ProcessPacket(PacketHelper * packet, bool send);
	virtual u_short GetTunnelPort() {return this->TunnelPort;}
	virtual u_long GetSessionId() {return this->SessionId;}

private:
	u_short TunnelPort;
	u_int SessionId;
	BarbaClientHttpCourier* Courier;
};

