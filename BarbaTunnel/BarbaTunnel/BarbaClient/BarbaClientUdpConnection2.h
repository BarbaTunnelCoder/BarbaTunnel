#pragma once
#include "BarbaClientConnection.h"
#include "BarbaCourierUdpClient.h"

class BarbaClientUdpConnection : public BarbaClientConnection
{
public:
	class Courier : public BarbaCourierUdpClient
	{
		//void ReciveData(PacketHelper* packet, bool outbound);
		void SendPacketToOutbound(PacketHelper* packet) override;
	};
	
public:
	BarbaClientUdpConnection(BarbaClientConfig* config);
	virtual ~BarbaClientUdpConnection(void);
	bool ProcessOutboundPacket(PacketHelper* packet) override;
	bool ProcessInboundPacket(PacketHelper* packet) override;

private:
	Courier* GetCourier() { return _Courier; }
	Courier* _Courier;
};

