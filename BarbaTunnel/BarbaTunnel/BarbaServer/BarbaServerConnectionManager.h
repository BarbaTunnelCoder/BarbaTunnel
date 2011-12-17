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
	void Initialize(IpRange* virtualIpRange);
	BarbaServerConnection* FindByVirtualIp(DWORD ip);
	BarbaServerConnection* FindBySessionId(DWORD sessionId);
	BarbaServerConnection* CreateConnection(PacketHelper* packet, BarbaServerConfigItem* configItem);
	BarbaServerHttpConnection* CreateHttpConnection(BarbaServerConfigItem* configItem, u_long clientIp, u_short tunnelPort, u_long sessionId);

private:
	BarbaServerVirtualIpManager VirtualIpManager;
};

