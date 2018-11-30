#pragma once
#include "BarbaConnection.h"
#include "SimpleSafeList.h"

class BarbaConnectionManager
{
public:
	BarbaConnectionManager(void);
	virtual ~BarbaConnectionManager(void);
	virtual void Dispose();
	virtual void RemoveConnection(BarbaConnection* conn);
	void AddConnection(BarbaConnection* conn);
	void CleanTimeoutConnections();
	BarbaConnection* FindByConfig(BarbaConfig* config);
	SimpleSafeList<BarbaConnection*> Connections;

private:
	void DoIntervalCheck();
	DWORD LastIntervalTime;

};

