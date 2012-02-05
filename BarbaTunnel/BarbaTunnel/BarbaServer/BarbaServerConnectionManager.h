#pragma once
#include "SimpleSafeList.h"
#include "BarbaServerVirtualIpManager.h"
#include "BarbaServerConnection.h"
#include "BarbaConnectionManager.h"
#include "BarbaServerHttpConnection.h"

class BarbaServerConnectionManager : public BarbaConnectionManager
{
public:
	BarbaServerConnectionManager(void);
	virtual ~BarbaServerConnectionManager(void);
	virtual void RemoveConnection(BarbaConnection* conn);
	void Initialize(IpRange* virtualIpRange);
	BarbaServerConnection* FindByVirtualIp(DWORD ip);
	BarbaServerConnection* FindBySessionId(DWORD sessionId);
	BarbaServerConnection* CreateConnection(PacketHelper* packet, BarbaServerConfig* config);
	BarbaServerHttpConnection* CreateHttpConnection(BarbaServerConfig* config, u_long clientIp, u_short tunnelPort, u_long sessionId);

private:
	BarbaServerVirtualIpManager VirtualIpManager;
};

