#pragma once
#include "BarbaCourierDatagram.h"
#include "BarbaPortRange.h"

class BarbaCourierUdpClient : public BarbaCourierDatagram
{
public:
	class CreateStrcutUdp : public CreateStrcut
	{
	public:
		CreateStrcutUdp() {RemoteIp=0; PortRange=NULL;}
		DWORD RemoteIp;
		BarbaPortRange* PortRange;
	};

	BarbaCourierUdpClient(CreateStrcutUdp* cs);
	virtual ~BarbaCourierUdpClient(void);
	CreateStrcutUdp* GetCreateStruct() {return (CreateStrcutUdp*)BarbaCourierDatagram::GetCreateStruct();}
	bool ProcessInboundPacket(PacketHelper* packet);

protected:
	void SendChunkToOutbound(BarbaBuffer* chunk) final override;
	virtual void SendUdpPacketToOutbound(DWORD remoteIp, u_short srcPort, u_short desPort, BarbaBuffer* payLoad)=0;
};

