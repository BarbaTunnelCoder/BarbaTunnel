#pragma once
#include "BarbaConnection.h"
#include "BarbaClientConfig.h"

//BarbaClientConnection
class BarbaClientConnection : public BarbaConnection
{
public:
	explicit BarbaClientConnection(BarbaClientConfig* config);
	virtual ~BarbaClientConnection(){}
	virtual void ReportNewConnection();
	virtual BarbaBuffer* GetKey();
	virtual LPCTSTR GetName();
	virtual BarbaModeEnum GetMode();
	u_long GetServerIp();
	BarbaClientConfig* GetConfigItem() {return this->Config;}
	
protected:
	BarbaClientConfig* Config;
};
