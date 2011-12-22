#pragma once
#include "BarbaConnection.h"
#include "SimpleSafeList.h"

class BarbaConnectionManager
{
public:
	BarbaConnectionManager(void);
	virtual ~BarbaConnectionManager(void);
	virtual void Dispose();
	SimpleSafeList<BarbaConnection*> Connections;
	void RemoveConnection(BarbaConnection* conn);
	void AddConnection(BarbaConnection* conn);
	void CleanTimeoutConnections();
	BarbaConnection* FindByPacketToProcess(PacketHelper* packet);

private:
	void DoIntervalCheck();
	DWORD LastIntervalTime;

};

