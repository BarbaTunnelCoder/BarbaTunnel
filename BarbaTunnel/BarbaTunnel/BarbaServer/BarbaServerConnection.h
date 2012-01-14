#pragma once
#include "BarbaConnection.h"
#include "BarbaServerConfig.h"

//BarbaServerConnection
class BarbaServerConnection : public BarbaConnection
{
public:
	explicit BarbaServerConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp);
	virtual ~BarbaServerConnection(){}
	virtual BarbaModeEnum GetMode();
	virtual std::vector<BYTE>* GetKey();
	virtual LPCTSTR GetName();
	u_long GetClientVirtualIp();
	void ReportNewConnection();
	BarbaServerConfig* GetConfigItem() {return this->Config;}

protected:
	u_long ClientVirtualIp;
	u_long ClientIp;
	BarbaServerConfig* Config;
};


