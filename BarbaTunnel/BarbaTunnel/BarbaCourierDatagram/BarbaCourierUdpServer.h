#pragma once
#include "General.h"
#include "BarbaCourierDatagram.h"

class BarbaCourierUdpServer : public BarbaCourierDatagram
{
public:
	struct CreateStrcutUdp : public CreateStrcut
	{
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
		int NextClientPortIndex;

	public:
		PortManager();
		void AddPort(u_short serverPort, u_short clientPort);
		void FindPort(u_short* serverPort, u_short* clientPort);
	};
	PortManager portManager;

public:
	BarbaCourierUdpServer(CreateStrcutUdp* cs);
	virtual ~BarbaCourierUdpServer(void);
	bool ProcessInboundPacket(PacketHelper* packet);

protected:
	void SendChunkToOutbound(BarbaBuffer* chunk) final override;
	virtual void SendUdpPacketToOutbound(DWORD remoteIp, u_short srcPort, u_short desPort, BarbaBuffer* payLoad) = 0;
};

