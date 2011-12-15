#pragma once
#include "BarbaServerConnection.h"
#include "BarbaServerHttpCourier.h"

class BarbaServerHttpConnection :
	public BarbaServerConnection
{
public:
	explicit BarbaServerHttpConnection(u_long sessionId);
	virtual ~BarbaServerHttpConnection(void);
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer);
	virtual u_long GetSessionId();
	virtual BarbaModeEnum GetMode();
	bool AddSocket(BarbaSocket* Socket, bool isOutgoing);

private:
	BarbaServerHttpCourier HttpCourier;
	u_long SessionId;
};

