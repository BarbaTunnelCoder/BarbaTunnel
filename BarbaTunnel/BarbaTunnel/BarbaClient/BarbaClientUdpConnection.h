#pragma once
#include "BarbaClientConnection.h"
#include "BarbaCourierUdpClient.h"

class BarbaClientUdpConnection : public BarbaClientConnection
{
public:
	class Courier : public BarbaCourierUdpClient
	{
	public:
		Courier(BarbaClientUdpConnection* connection, CreateStrcutUdp* cs);
		void ReceiveData(BarbaBuffer* data) override;
		void SendPacketToOutbound(PacketHelper* packet) override;
		void Encrypt(BYTE* data, size_t dataSize, size_t index) override;
		void Decrypt(BYTE* data, size_t dataSize, size_t index) override;

	private:
		BarbaClientUdpConnection* _Connection;
	};
	
public:
	BarbaClientUdpConnection(BarbaClientConfig* config);
	virtual ~BarbaClientUdpConnection(void);
	bool ProcessOutboundPacket(PacketHelper* packet) override;
	bool ProcessInboundPacket(PacketHelper* packet) override;

private:
	ether_header LastEtherHeader;
	Courier* GetCourier() { return _Courier; }
	Courier* _Courier;
};

