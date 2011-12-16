#pragma once
#include "BarbaServerConnection.h"

class BarbaServerRedirectConnection : public BarbaServerConnection
{
public:
	explicit BarbaServerRedirectConnection(LPCTSTR connectionName, BarbaKey* barbaKey, u_short realPort, BarbaModeEnum mode);
	virtual ~BarbaServerRedirectConnection(void);
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packet);
	virtual BarbaModeEnum GetMode();

private:
	BarbaModeEnum Mode;
	u_short RealPort;
};

