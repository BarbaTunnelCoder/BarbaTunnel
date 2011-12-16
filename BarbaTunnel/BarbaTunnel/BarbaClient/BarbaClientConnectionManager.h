#pragma once
#include "BarbaClientConnection.h"
#include "BarbaConnectionManager.h"

class BarbaClientConnectionManager : public BarbaConnectionManager
{
public:
	BarbaClientConnectionManager(void);
	virtual ~BarbaClientConnectionManager(void);
	BarbaClientConnection* FindByPacketToProcess(PacketHelper* packet);
	BarbaClientConnection* CreateConnection(PacketHelper* packet, BarbaClientConfig* config, BarbaClientConfigItem* configItem);

};

