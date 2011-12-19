/*
 * BarbaServerHttpCourier class
 * this class just used for HTTP-Tunneling
 */

#pragma once
#include "General.h"
#include "BarbaCourier.h"

class BarbaServerHttpConnection;
class BarbaServerHttpCourier : public BarbaCourierServer
{
public:
	explicit BarbaServerHttpCourier(u_short maxConnection, BarbaServerHttpConnection* httpConnection);
	virtual void Receive(BYTE* buffer, size_t bufferCount);
	void SendPacket(PacketHelper* packet);
	GUID SessionID;


protected:
	virtual ~BarbaServerHttpCourier(void);
	BarbaServerHttpConnection* HttpConnection;
};

