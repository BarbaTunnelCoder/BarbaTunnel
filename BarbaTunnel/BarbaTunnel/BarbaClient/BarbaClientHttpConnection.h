#pragma once
#include "BarbaClientConnection.h"
#include "BarbaClientHttpCourier.h"
class BarbaClientHttpConnection : public BarbaClientConnection
{
public:
	explicit BarbaClientHttpConnection(LPCTSTR connectionName, BarbaKey* key, DWORD remoteIp, u_short remotePort, u_short maxConnenction);
	virtual ~BarbaClientHttpConnection(void);

private:
	u_long SessionId;
	BarbaClientHttpCourier Courier;
};

