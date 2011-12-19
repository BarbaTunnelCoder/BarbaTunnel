#pragma once
#include "General.h"
#include "BarbaCourier.h"

class BarbaClientHttpConnection;
class BarbaClientHttpCourier : public BarbaCourierClient
{
public:
	explicit BarbaClientHttpCourier(u_short maxConnenction, DWORD remoteIp, u_short remotePort, LPCSTR fakeHttpGetTemplate, LPCSTR fakeHttpPostTemplate, BarbaClientHttpConnection* httpConnection);
	virtual void Receive(BYTE* buffer, size_t bufferCount);
	void SendPacket(PacketHelper* packet);

protected:
	BarbaClientHttpConnection* HttpConnection;
	virtual ~BarbaClientHttpCourier(void);
};

