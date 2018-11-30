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
		void SendUdpPacketToOutbound(DWORD remoteIp, u_short srcPort, u_short desPort, BarbaBuffer* payLoad) override;
		void Encrypt(BYTE* data, size_t dataSize, size_t index) override;
		void Decrypt(BYTE* data, size_t dataSize, size_t index) override;

	private:
		BarbaClientUdpConnection* _Connection;
	};
	
public:
	BarbaClientUdpConnection(BarbaClientConfig* config, PacketHelper* initPacket);
	virtual ~BarbaClientUdpConnection(void);
	bool ProcessOutboundPacket(PacketHelper* packet) override;
	bool ProcessInboundPacket(PacketHelper* packet) override;
	void Init() override;
	u_long GetSessionId() override;

private:
	DWORD LocalIp;
	Courier* GetCourier() { return _Courier; }
	Courier* _Courier;
};

