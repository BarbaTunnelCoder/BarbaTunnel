#pragma once
#include "BarbaConnection.h"
#include "BarbaClientConfig.h"

//BarbaClientConnection
class BarbaClientConnection : public BarbaConnection
{
public:
	explicit BarbaClientConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem);
	virtual ~BarbaClientConnection(){}
	virtual void ReportNewConnection();
	virtual std::vector<BYTE>* GetKey();
	virtual LPCTSTR GetName();
	virtual BarbaModeEnum GetMode();
	u_long GetServerIp();

protected:
	BarbaClientConfig* Config;
	BarbaClientConfigItem* ConfigItem;
};
