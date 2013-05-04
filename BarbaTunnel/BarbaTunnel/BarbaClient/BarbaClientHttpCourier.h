#pragma once
#include "General.h"
#include "BarbaCourierClient.h"

class BarbaClientHttpConnection;
class BarbaClientHttpCourier : public BarbaCourierClient
{
public:
	explicit BarbaClientHttpCourier(BarbaCourier::CreateStrcutBag* cs, DWORD remoteIp, u_short remotePort, BarbaClientHttpConnection* httpConnection);
	virtual void Receive(BarbaBuffer* data);
	virtual void Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt);
	void SendPacket(PacketHelper* packet);

protected:
	BarbaClientHttpConnection* HttpConnection;
	virtual void GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew);
	virtual ~BarbaClientHttpCourier(void);
	std::tstring GetHttpPostTemplate(bool bombardMode) override;
	std::tstring GetHttpGetTemplate(bool bombardMode) override;

private:
	u_long SessionId;
};

