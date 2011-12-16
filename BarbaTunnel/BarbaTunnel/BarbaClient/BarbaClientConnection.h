#pragma once
#include "BarbaConnection.h"
#include "BarbaClientConfig.h"

//BarbaClientConnection
class BarbaClientConnection : public BarbaConnection
{
public:
	explicit BarbaClientConnection(LPCTSTR connectionName, BarbaKey* key);
	virtual ~BarbaClientConnection(){}
	virtual void ReportNewConnection();

	BarbaClientConfig* Config;
	BarbaClientConfigItem* ConfigItem;
	u_short OrgClientPort;
	u_short ClientPort;
	u_short TunnelPort;
};
