#pragma once
#include "General.h"

class BarbaConnection
{
public:
	explicit BarbaConnection();
	virtual ~BarbaConnection(void);
	virtual bool ShouldProcessPacket(PacketHelper* packet)=0;
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)=0;
	virtual BarbaModeEnum GetMode()=0;
	virtual u_short GetTunnelPort()=0; //may 0 when protocol has not port
	virtual u_long GetSessionId() {return 0;} //may 0 when protocol has not session
	virtual void ReportNewConnection()=0;
	virtual BarbaKey* GetKey()=0;
	virtual LPCTSTR GetName()=0;
	DWORD GetLasNegotiationTime();

protected:
	void CryptPacket(PacketHelper* packet);
	void SetWorkingState(ULONG length, bool send);

private:
	DWORD LasNegotiationTime;
};

