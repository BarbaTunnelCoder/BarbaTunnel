#pragma once
#include "BarbaConnection.h"
#include "SimpleSafeList.h"

class BarbaConnectionManager
{
public:
	BarbaConnectionManager(void);
	virtual ~BarbaConnectionManager(void);
	SimpleSafeList<BarbaConnection*> Connections;
	void RemoveConnection(BarbaConnection* conn);
	void AddConnection(BarbaConnection* conn);
	void CleanTimeoutConnections();
	BarbaConnection* FindByPacketToProcess(PacketHelper* packet);

};

