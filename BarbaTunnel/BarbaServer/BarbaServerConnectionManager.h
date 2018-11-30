#pragma once
#include "SimpleSafeList.h"
#include "BarbaServerVirtualIpManager.h"
#include "BarbaServerConnection.h"
#include "BarbaConnectionManager.h"
#include "BarbaServerTcpConnectionBase.h"

class BarbaServerConnectionManager : public BarbaConnectionManager
{
public:
	BarbaServerConnectionManager(void);
	virtual ~BarbaServerConnectionManager(void);
	virtual void RemoveConnection(BarbaConnection* conn);
	void Initialize(IpRange* virtualIpRange);
	BarbaServerConnection* FindByVirtualIp(DWORD ip);
	BarbaServerConnection* FindBySessionId(u_long sessionId);
	BarbaServerConnection* CreateConnection(BarbaServerConfig* config, PacketHelper* packet);
	BarbaServerTcpConnectionBase* CreateTcpConnection(BarbaServerConfig* config, u_long clientIp, LPCTSTR requestData);

private:
	BarbaServerVirtualIpManager VirtualIpManager;
};

