#pragma once
#include "BarbaServerTcpConnectionBase.h"
#include "BarbaCourierHttpServer.h"

class BarbaServerHttpConnection : public BarbaServerTcpConnectionBase
{
public:
	class Courier : public BarbaCourierHttpServer
	{
	public:
		explicit Courier(CreateStrcutHttp* cs, BarbaServerHttpConnection* connection);
		virtual ~Courier(void);
		void Receive(BarbaBuffer* data) override;
		void Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt) override;

	private:
		std::tstring GetHttpPostReplyRequest(bool bombardMode) override;
		std::tstring GetHttpGetReplyRequest(bool bombardMode) override;
		void GetFakeFile(TCHAR* filename, std::tstring* contentType, BarbaBuffer* fakeFileHeader) override;
		BarbaServerHttpConnection* _Connection;
	};

public:
	explicit BarbaServerHttpConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp);
	virtual ~BarbaServerHttpConnection(void);
	void Init(LPCTSTR requestData) override;
};

