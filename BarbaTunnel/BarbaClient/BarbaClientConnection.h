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
	BarbaClientConfig* GetConfig() { return (BarbaClientConfig*)BarbaConnection::GetConfig(); }
};
