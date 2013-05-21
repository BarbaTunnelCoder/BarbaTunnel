#pragma once
#include "BarbaServerTcpConnectionBase.h"
#include "BarbaCourierTcpServer.h"

class BarbaServerTcpConnection : public BarbaServerTcpConnectionBase
{
public:
	class Courier : public BarbaCourierTcpServer
	{
	public:
		explicit Courier(CreateStrcutTcp* cs, BarbaServerTcpConnection* connection);
		void Receive(BarbaBuffer* data) override;
		void Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt) override;

	protected:
		virtual ~Courier(void);
		BarbaServerTcpConnection* _Connection;
	};

public:
	explicit BarbaServerTcpConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp);
	virtual ~BarbaServerTcpConnection(void);
	void Init(LPCTSTR requestData) override;
};

