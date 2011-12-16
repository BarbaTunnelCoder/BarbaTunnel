#pragma once
#include "BarbaClientConnection.h"
#include "BarbaClientHttpCourier.h"
class BarbaClientHttpConnection : public BarbaClientConnection
{
public:
	explicit BarbaClientHttpConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem, u_short tunnelPort);
	virtual ~BarbaClientHttpConnection(void);

private:
	u_long SessionId;
	BarbaClientHttpCourier Courier;
};

