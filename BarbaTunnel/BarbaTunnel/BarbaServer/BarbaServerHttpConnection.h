#pragma once
#include "BarbaServerConnection.h"
#include "BarbaServerHttpCourier.h"

class BarbaServerHttpConnection :
	public BarbaServerConnection
{
public:
	explicit BarbaServerHttpConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, u_short clientIp, u_long sessionId);
	virtual ~BarbaServerHttpConnection(void);
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer);
	virtual u_long GetSessionId();
	virtual bool ShouldProcessPacket(PacketHelper* packet);
	bool AddSocket(BarbaSocket* Socket, bool isOutgoing);

private:
	BarbaServerHttpCourier HttpCourier;
	u_long SessionId;
};

