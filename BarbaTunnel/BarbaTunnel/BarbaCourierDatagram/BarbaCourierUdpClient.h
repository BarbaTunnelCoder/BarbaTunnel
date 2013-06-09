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
	virtual void SendPacketToOutbound(PacketHelper* packet)=0;
};

