#pragma once
#include "General.h"
#include "BarbaConfig.h"

class BarbaConnection
{
public:
	explicit BarbaConnection(BarbaConfig* config);
	virtual ~BarbaConnection(void);
	virtual bool ProcessOutboundPacket(PacketHelper * packet)=0;
	virtual bool ProcessInboundPacket(PacketHelper * packet)=0;
	virtual u_long GetSessionId()=0;
	virtual void ReportNewConnection()=0;
	virtual void Init();
	virtual size_t GetId() {return this->ConnectionId;}
	BarbaConfig* GetConfig() { return _Config; }
	void CryptData(BYTE* data, size_t dataSize, size_t index, bool encrypt);
	DWORD GetLasNegotiationTime();
	void Log2(LPCTSTR format, ...);
	void Log3(LPCTSTR format, ...);


protected:
	void LogImpl(int level, LPCTSTR format, va_list _ArgList);
	void EncryptPacket(PacketHelper* packet);
	void DecryptPacket(PacketHelper* packet);
	bool SendPacketToOutbound(PacketHelper* packet);
	bool SendPacketToInbound(PacketHelper* packet);
	void SetWorkingState(size_t length, bool send);

private:
	DWORD LasNegotiationTime;
	size_t ConnectionId;
	BarbaConfig* _Config;
};

