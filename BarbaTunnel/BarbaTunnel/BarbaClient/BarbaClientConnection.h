#pragma once
#include "BarbaConnection.h"
#include "BarbaClientConfig.h"

//BarbaClientConnection
class BarbaClientConnection : public BarbaConnection
{
public:
	explicit BarbaClientConnection(BarbaClientConfig* config);
	virtual ~BarbaClientConnection(){}
	void ReportNewConnection() override;
	BarbaBuffer* GetKey() override;
	BarbaModeEnum GetMode() override;
	u_long GetServerIp();
	BarbaClientConfig* GetConfigItem() {return this->Config;}
	
protected:
	BarbaClientConfig* Config;
};
