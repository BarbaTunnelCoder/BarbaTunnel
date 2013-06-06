#pragma once
#include "BarbaClientConnection.h"
#include "BarbaConnectionManager.h"

class BarbaClientConnectionManager : public BarbaConnectionManager
{
public:
	BarbaClientConnectionManager(void);
	virtual ~BarbaClientConnectionManager(void);
	BarbaClientConnection* CreateConnection(PacketHelper* packet, BarbaClientConfig* config);
};

