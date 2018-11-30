#pragma once
#include "BarbaCourierDatagram.h"
#include "BarbaPortRange.h"

class BarbaCourierUdpClient : public BarbaCourierDatagram
{
public:
	static const int MaxKeepAlivePortsCount = 500;
	class CreateStrcutUdp : public CreateStrcut
	{
	public:
		CreateStrcutUdp() {RemoteIp=0; PortRange=NULL; KeepAliveInterval=0; KeepAlivePortsCount=100;}
		DWORD RemoteIp;
		BarbaPortRange* PortRange;
		DWORD KeepAliveInterval;
		DWORD KeepAlivePortsCount;
	};

	BarbaCourierUdpClient(CreateStrcutUdp* cs);
	virtual ~BarbaCourierUdpClient(void);
	CreateStrcutUdp* GetCreateStruct() {return (CreateStrcutUdp*)BarbaCourierDatagram::GetCreateStruct();}
	bool ProcessInboundPacket(PacketHelper* packet);

protected:
	void Timer() override;
	void SendChunkToOutbound(BarbaBuffer* chunk) override;
	bool PreReceiveDataControl(BarbaBuffer* data) override;
	virtual void SendUdpPacketToOutbound(DWORD remoteIp, u_short srcPort, u_short desPort, BarbaBuffer* payLoad)=0;

private:
	DWORD LastKeepAliveTime;
	size_t LastKeepAliveSentChunkCount;
	size_t SentChunkCount;
	void CheckKeepAlive();
	void SendInitCommand();
};

