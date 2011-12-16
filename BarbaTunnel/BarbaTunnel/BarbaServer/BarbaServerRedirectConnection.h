#pragma once
#include "BarbaServerConnection.h"

class BarbaServerRedirectConnection : public BarbaServerConnection
{
public:
	explicit BarbaServerRedirectConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, 
		u_long clientIp, u_short clientPort, u_short tunnelPort);
	virtual ~BarbaServerRedirectConnection(void);
	virtual bool ShouldProcessPacket(PacketHelper* packet);
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packet);
	virtual u_short GetTunnelPort();
	u_short GetRealPort();

private:
	u_short ClientPort;
	u_short TunnelPort;
};

