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
	virtual std::vector<BYTE>* GetKey()=0;
	virtual LPCTSTR GetName()=0;
	virtual size_t GetId() {return this->ConnectionId;}
	void EncryptData(std::vector<BYTE>* data);
	void DecryptData(std::vector<BYTE>* data);
	size_t GetLasNegotiationTime();

protected:
	void EncryptPacket(PacketHelper* packet);
	void DecryptPacket(PacketHelper* packet);
	bool SendPacketToOutbound(PacketHelper* packet);
	bool SendPacketToInbound(PacketHelper* packet);
	void SetWorkingState(size_t length, bool send);

private:
	size_t LasNegotiationTime;
	size_t ConnectionId;
};

