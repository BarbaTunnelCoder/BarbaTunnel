#pragma once
#include "BarbaClientConnection.h"
class BarbaClientRedirectConnection :
	public BarbaClientConnection
{
public:
	explicit BarbaClientRedirectConnection(LPCTSTR connectionName, BarbaKey* key, BarbaModeEnum mode);
	virtual ~BarbaClientRedirectConnection(void);
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer);
	virtual BarbaModeEnum GetMode();

private:
	BarbaModeEnum Mode;
};

