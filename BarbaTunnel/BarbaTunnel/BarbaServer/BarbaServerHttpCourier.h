/*
 * BarbaServerHttpCourier class
 * this class just used for HTTP-Tunneling
 */

#pragma once
#include "General.h"
#include "BarbaCourierServer.h"

class BarbaServerHttpConnection;
class BarbaServerHttpCourier : public BarbaCourierServer
{
public:
	explicit BarbaServerHttpCourier(BarbaCourierCreateStrcut* cs, BarbaServerHttpConnection* httpConnection);
	virtual void Receive(BYTE* buffer, size_t bufferCount);
	void SendPacket(PacketHelper* packet);
	//virtual bool GetFakeFile(TCHAR* filename, u_int* fileSize, SimpleBuffer* fakeFileHeader, bool createNew);

protected:
	virtual ~BarbaServerHttpCourier(void);
	BarbaServerHttpConnection* HttpConnection;
};

