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
	virtual void Crypt(BYTE* data, size_t dataLen, bool encrypt);
	void SendPacket(PacketHelper* packet);

protected:
	virtual void GetFakeFile(TCHAR* filename, std::tstring* contentType, u_int* fileSize, std::vector<BYTE>* fakeFileHeader, bool createNew);
	virtual ~BarbaServerHttpCourier(void);
	BarbaServerHttpConnection* HttpConnection;
};

