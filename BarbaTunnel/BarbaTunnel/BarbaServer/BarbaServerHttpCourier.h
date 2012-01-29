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
	virtual void Receive(std::vector<BYTE>* data);
	virtual void Crypt(std::vector<BYTE>* data, bool encrypt);
	void SendPacket(PacketHelper* packet);

protected:
	virtual void GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, std::vector<BYTE>* fakeFileHeader, bool createNew);
	virtual ~BarbaServerHttpCourier(void);
	BarbaServerHttpConnection* HttpConnection;
};

