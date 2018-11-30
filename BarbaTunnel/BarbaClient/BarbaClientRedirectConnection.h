#pragma once
#include "BarbaClientConnection.h"
class BarbaClientRedirectConnection : public BarbaClientConnection
{
public:
	explicit BarbaClientRedirectConnection(BarbaClientConfig* config, PacketHelper* initPacket);
	virtual ~BarbaClientRedirectConnection(void);
	bool ProcessOutboundPacket(PacketHelper* packet) override;
	bool ProcessInboundPacket(PacketHelper* packet) override;
	u_long GetSessionId() override;
	u_short GetRealPort();

private:
	u_short ClientPort;
	u_short TunnelPort;
};

