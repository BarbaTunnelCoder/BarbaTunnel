#pragma once
#include "General.h"

class BarbaConnection
{
public:
	explicit BarbaConnection();
	virtual ~BarbaConnection(void);
	virtual bool ShouldProcessPacket(PacketHelper* packet)=0;
	virtual bool ProcessPacket(PacketHelper* packet, bool send)=0;
	virtual BarbaModeEnum GetMode()=0;
	virtual u_short GetTunnelPort()=0; //may 0 when protocol has not port
	virtual u_long GetSessionId() {return 0;} //may 0 when protocol has not session
	virtual void ReportNewConnection()=0;
	virtual SimpleBuffer* GetKey()=0;
	virtual LPCTSTR GetName()=0;
	virtual u_int GetId() {return this->ConnectionId;}
	void CryptData(BYTE* data, size_t dataLen);
	u_int GetLasNegotiationTime();

protected:
	void CryptPacket(PacketHelper* packet);
	bool SendPacketToAdapter(PacketHelper* packet);
	bool SendPacketToMstcp(PacketHelper* packet);
	void SetWorkingState(ULONG length, bool send);

private:
	u_int LasNegotiationTime;
	u_int ConnectionId;
};

