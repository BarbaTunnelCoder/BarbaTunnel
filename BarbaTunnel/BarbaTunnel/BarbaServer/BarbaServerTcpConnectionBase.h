#pragma once
#include "BarbaServerConnection.h"
#include "BarbaCourierTcpServer.h"

class BarbaServerTcpConnectionBase : public BarbaServerConnection
{
public:
	explicit BarbaServerTcpConnectionBase(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp);
	virtual ~BarbaServerTcpConnectionBase(void);
	bool ShouldProcessPacket(PacketHelper* packet) override;
	bool ProcessPacket(PacketHelper* packet, bool send) override;
	u_short GetTunnelPort() override {return 0;} 
	u_long GetSessionId() override;
	void AddSocket(BarbaSocket* Socket, LPCSTR requestString, LPCTSTR requestData);
	virtual void Init(LPCTSTR requestData)=0;

protected:
	void InitHelper(BarbaCourierTcpServer::CreateStrcutTcp* cs, LPCTSTR requestData);
	BarbaCourierTcpServer* GetCourier() {return _Courier;}
	u_long SessionId;
	u_long ClientLocalIp;
	BarbaCourierTcpServer* _Courier;
};

