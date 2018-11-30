#pragma once
#include "General.h"
#include "BarbaCourierDatagram.h"

class BarbaCourierUdpServer : public BarbaCourierDatagram
{
public:
	struct CreateStrcutUdp : public CreateStrcut
	{
		CreateStrcutUdp() { KeepAlivePortsCount = 50; };
		u_short KeepAlivePortsCount;
	};

private:
	class PortManager
	{
	private:
		struct PortPair
		{
			u_short ServerPort;
			u_short ClientPort;
		};
		BarbaArray<PortPair> PortPairs;
		int NextPortIndex;
		u_short MaxPortsCount;

	public:
		PortManager();
		void SetMaxPorts(u_short value);
		u_short GetMaxPorts();
		void AddPort(u_short serverPort, u_short clientPort);
		void FindPort(u_short* serverPort, u_short* clientPort);
	};
	PortManager portManager;

public:
	BarbaCourierUdpServer(CreateStrcutUdp* cs);
	virtual ~BarbaCourierUdpServer(void);
	bool ProcessInboundPacket(PacketHelper* packet);

protected:
	bool PreReceiveData(BarbaBuffer* data) override;
	bool PreReceiveDataControl(BarbaBuffer* data) override;
	void SendChunkToOutbound(BarbaBuffer* chunk) override;
	void SendInitRequest();
	virtual void SendUdpPacketToOutbound(DWORD remoteIp, u_short srcPort, u_short desPort, BarbaBuffer* payLoad) = 0;
};

