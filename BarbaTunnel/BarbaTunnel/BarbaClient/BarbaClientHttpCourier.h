#pragma once
#include "General.h"
#include "BarbaCourierClient.h"

class BarbaClientHttpConnection;
class BarbaClientHttpCourier : public BarbaCourierClient
{
public:
	explicit BarbaClientHttpCourier(BarbaCourierCreateStrcut* cs, DWORD remoteIp, u_short remotePort, BarbaClientHttpConnection* httpConnection);
	virtual void Receive(BYTE* buffer, size_t bufferCount);
	virtual void Crypt(BYTE* data, size_t dataLen, bool encrypt);
	void SendPacket(PacketHelper* packet);

protected:
	BarbaClientHttpConnection* HttpConnection;
	virtual void GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, std::vector<BYTE>* fakeFileHeader, bool createNew);
	virtual ~BarbaClientHttpCourier(void);

private:
	u_long SessionId;
};

