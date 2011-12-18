#pragma once
#include "BarbaConnection.h"
#include "BarbaServerConfig.h"

//BarbaServerConnection
class BarbaServerConnection : public BarbaConnection
{
public:
	explicit BarbaServerConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, u_long clientIp);
	virtual ~BarbaServerConnection(){}
	virtual BarbaModeEnum GetMode();
	virtual SimpleBuffer* GetKey();
	virtual LPCTSTR GetName();
	u_long GetClientVirtualIp();
	void ReportNewConnection();

protected:
	u_long ClientVirtualIp;
	u_long ClientIp;
	BarbaServerConfigItem* ConfigItem;
};


