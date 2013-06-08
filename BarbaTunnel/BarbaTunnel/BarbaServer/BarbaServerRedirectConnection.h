#pragma once
#include "BarbaServerConnection.h"

class BarbaServerRedirectConnection : public BarbaServerConnection
{
public:
	explicit BarbaServerRedirectConnection(BarbaServerConfig* config, u_long clientVirtualIp, PacketHelper* initPacket);
	virtual ~BarbaServerRedirectConnection(void);
	bool ProcessOutboundPacket(PacketHelper* packet) override;
	bool ProcessInboundPacket(PacketHelper* packet) override;
	u_short GetRealPort();
	u_long GetSessionId();
	
private:
	u_short ClientPort;
	u_short TunnelPort;
};

