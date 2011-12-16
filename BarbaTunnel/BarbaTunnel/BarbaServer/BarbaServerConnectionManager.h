#pragma once
#include "SimpleSafeList.h"
#include "BarbaServerVirtualIpManager.h"
#include "BarbaServerConnection.h"
#include "BarbaConnectionManager.h"

class BarbaServerConnectionManager : public BarbaConnectionManager
{
public:
	BarbaServerConnectionManager(void);
	virtual ~BarbaServerConnectionManager(void);
	void Initialize(IpRange* virtualIpRange);
	BarbaServerConnection* FindByVirtualIp(DWORD ip);
	BarbaServerConnection* FindBySessionId(DWORD sessionId);
	BarbaServerConnection* Find(DWORD clientIp, u_short clientPort, BarbaModeEnum mode);
	BarbaServerConnection* CreateConnection(PacketHelper* packet, BarbaServerConfigItem* configItem);


private:
	BarbaServerVirtualIpManager VirtualIpManager;
};

