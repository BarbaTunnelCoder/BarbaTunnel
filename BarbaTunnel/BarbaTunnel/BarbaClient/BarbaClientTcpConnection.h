#pragma once
#include "BarbaClientTcpConnectionBase.h"
#include "BarbaCourierTcpClient.h"

class BarbaClientTcpConnection : public BarbaClientTcpConnectionBase
{
public:
	class Courier : public BarbaCourierTcpClient
	{
	public:
		explicit Courier(BarbaClientTcpConnection* connection, CreateStrcutTcp* cs);
		virtual ~Courier(void);
		void Receive(BarbaBuffer* data) override;
		void Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt) override;

	private:
		BarbaClientTcpConnection* _Connection;
	};

public:
	explicit BarbaClientTcpConnection(BarbaClientConfig* config);
	virtual ~BarbaClientTcpConnection(void);
	void Init() override;

};

