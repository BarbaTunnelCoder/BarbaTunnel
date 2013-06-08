#pragma once
#include "BarbaConnection.h"
#include "BarbaServerConfig.h"

//BarbaServerConnection
class BarbaServerConnection : public BarbaConnection
{
public:
	explicit BarbaServerConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp);
	virtual ~BarbaServerConnection();
	void ReportNewConnection() override;
	u_long GetClientVirtualIp() { return ClientVirtualIp;}
	u_long GetClientIp() { return ClientIp; }
	BarbaServerConfig* GetConfig() {return (BarbaServerConfig*)BarbaConnection::GetConfig();}

protected:
	u_long ClientVirtualIp;
	u_long ClientIp;
};


