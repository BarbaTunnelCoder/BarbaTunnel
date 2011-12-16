#pragma once
#include "BarbaClientConnection.h"
#include "BarbaConnectionManager.h"

class BarbaClientConnectionManager : public BarbaConnectionManager
{
public:
	BarbaClientConnectionManager(void);
	virtual ~BarbaClientConnectionManager(void);
	BarbaClientConnection* Find(DWORD serverIp, u_char tunnelProtocol, u_short clientPort);
	BarbaClientConnection* FindByConfigItem(BarbaClientConfigItem* configItem, u_short clientPort);
	BarbaClientConnection* CreateConnection(PacketHelper* packet, BarbaClientConfig* config, BarbaClientConfigItem* configItem);

};

