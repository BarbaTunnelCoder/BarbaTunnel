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
	explicit BarbaServerHttpCourier(BarbaCourier::CreateStrcutBag* cs, BarbaServerHttpConnection* httpConnection);
	virtual void Receive(BarbaBuffer* data);
	virtual void Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt);
	void SendPacket(PacketHelper* packet);

protected:
	std::tstring GetHttpPostReplyRequest(bool bombardMode) override;
	std::tstring GetHttpGetReplyRequest(bool bombardMode) override;
	void GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew) override;
	virtual ~BarbaServerHttpCourier(void);

	BarbaServerHttpConnection* HttpConnection;
};

