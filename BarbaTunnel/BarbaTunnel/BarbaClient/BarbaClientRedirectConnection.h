#pragma once
#include "BarbaClientConnection.h"
class BarbaClientRedirectConnection :
	public BarbaClientConnection
{
public:
	explicit BarbaClientRedirectConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem, u_short clientPort, u_short tunnelPort);
	virtual ~BarbaClientRedirectConnection(void);
	virtual bool ProcessPacket(PacketHelper* packet, bool send);
	virtual bool ShouldProcessPacket(PacketHelper* packet);
	virtual u_short GetTunnelPort();
	u_short GetRealPort();

private:
	u_short ClientPort;
	u_short TunnelPort;
};

