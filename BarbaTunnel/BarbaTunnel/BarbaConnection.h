#pragma once
#include "General.h"
#include "PacketHelper.h"

class BarbaConnection
{
public:
	explicit BarbaConnection(LPCTSTR connectionName, BarbaKey* key);
	virtual ~BarbaConnection(void);
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packet)=0;
	virtual BarbaModeEnum GetMode()=0;
	virtual u_long GetSessionId() {return 0;}
	virtual void ReportNewConnection()=0;
	DWORD GetLasNegotiationTime();

protected:
	void CryptPacket(PacketHelper* packet);
	void SetWorkingState(ULONG length, bool send);
	LPCTSTR GetConnectionName();

private:
	DWORD LasNegotiationTime;
	BarbaKey Key;
	TCHAR ConnectionName[BARBA_MAX_CONFIGNAME];
};

