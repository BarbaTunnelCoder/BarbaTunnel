#pragma once
#include "BarbaServerConnection.h"
#include "BarbaCourierUdpServer.h"

class BarbaServerUdpConnection : public BarbaServerConnection
{
public:
	class Courier : public BarbaCourierUdpServer
	{
	public:
		Courier(BarbaServerUdpConnection* connection, CreateStrcutUdp* cs);
		void ReceiveData(BarbaBuffer* data) override;
		void SendUdpPacketToOutbound(DWORD remoteIp, u_short srcPort, u_short desPort, BarbaBuffer* payLoad) override;
		void Encrypt(BYTE* data, size_t dataSize, size_t index) override;
		void Decrypt(BYTE* data, size_t dataSize, size_t index) override;

	private:
		BarbaServerUdpConnection* _Connection;
	};
	
public:
	BarbaServerUdpConnection(BarbaServerConfig* config, u_long clientVirtualIp, PacketHelper* initPacket);
	virtual ~BarbaServerUdpConnection(void);
	bool ProcessOutboundPacket(PacketHelper* packet) override;
	bool ProcessInboundPacket(PacketHelper* packet) override;
	void Init() override;
	u_long GetSessionId() override;

private:
	u_long ClientLocalIp;
	iphdr LastOutboundIpHeader;
	Courier* GetCourier() { return _Courier; }
	Courier* _Courier;
};

