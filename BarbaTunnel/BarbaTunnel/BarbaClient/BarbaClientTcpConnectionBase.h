#pragma once
#include "BarbaClientConnection.h"
#include "BarbaCourierTcpClient.h"

class BarbaClientTcpConnectionBase : public BarbaClientConnection
{
public:
	explicit BarbaClientTcpConnectionBase(BarbaClientConfig* config);
	virtual ~BarbaClientTcpConnectionBase(void);
	bool ProcessOutboundPacket(PacketHelper * packet) override;
	bool ProcessInboundPacket(PacketHelper * packet) override;
	u_long GetSessionId() override {return this->SessionId;}
	virtual void Init()=0;

protected:
	void ReceiveData(BarbaBuffer* data);
	void InitHelper(BarbaCourierTcpClient::CreateStrcutTcp* cs);
	BarbaCourierTcpClient* GetCourier() { return _Courier; }
	BarbaCourierTcpClient* _Courier;

private:
	u_long SessionId;
};

