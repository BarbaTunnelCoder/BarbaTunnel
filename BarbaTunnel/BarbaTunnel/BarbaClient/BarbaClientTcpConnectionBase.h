#pragma once
#include "BarbaClientConnection.h"
#include "BarbaCourierTcpClient.h"

class BarbaClientTcpConnectionBase : public BarbaClientConnection
{
public:
	explicit BarbaClientTcpConnectionBase(BarbaClientConfig* config);
	virtual ~BarbaClientTcpConnectionBase(void);
	bool ShouldProcessPacket(PacketHelper* packet) override;
	bool ProcessPacket(PacketHelper * packet, bool send) override;
	u_short GetTunnelPort() override {return 0;}
	u_long GetSessionId() override {return this->SessionId;}
	virtual void Init()=0;

protected:
	void InitHelper(BarbaCourierTcpClient::CreateStrcutTcp* cs);
	BarbaCourierTcpClient* GetCourier() { return _Courier; }
	BarbaCourierTcpClient* _Courier;

private:
	u_long SessionId;
};

