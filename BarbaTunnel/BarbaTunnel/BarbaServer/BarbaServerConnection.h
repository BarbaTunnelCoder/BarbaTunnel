#pragma once
#include "BarbaConnection.h"
#include "BarbaServerConfig.h"

//BarbaServerConnection
class BarbaServerConnection : public BarbaConnection
{
public:
	explicit BarbaServerConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp);
	virtual ~BarbaServerConnection();
	BarbaModeEnum GetMode() override;
	BarbaBuffer* GetKey() override;
	void ReportNewConnection() override;
	u_long GetClientVirtualIp();
	BarbaServerConfig* GetConfigItem() {return this->Config;}

protected:
	u_long ClientVirtualIp;
	u_long ClientIp;
	BarbaServerConfig* Config;
};


