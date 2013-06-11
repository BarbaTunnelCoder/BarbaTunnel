#include "StdAfx.h"
#include "BarbaClientTcpConnection.h"
#include "BarbaClientApp.h"


BarbaClientTcpConnection::BarbaClientTcpConnection(BarbaClientConfig* config) 
	: BarbaClientTcpConnectionBase(config)
{
	_Courier = NULL;
}

BarbaClientTcpConnection::~BarbaClientTcpConnection(void)
{
}

void BarbaClientTcpConnection::Init()
{
	BarbaCourierTcpClient::CreateStrcutTcp* cs = new BarbaCourierTcpClient::CreateStrcutTcp();
	InitHelper(cs);
	_Courier = new Courier(this, cs);
	_Courier->Init();
}

BarbaClientTcpConnection::Courier::Courier(BarbaClientTcpConnection* connection, BarbaCourierTcpClient::CreateStrcutTcp* cs)
	: BarbaCourierTcpClient(cs)
	, _Connection(connection)
{
}


BarbaClientTcpConnection::Courier::~Courier(void)
{
}

void BarbaClientTcpConnection::Courier::Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt)
{
	if (IsDisposing()) 
		return; 

	_Connection->CryptData(data, dataSize, index, encrypt);
}

void BarbaClientTcpConnection::Courier::Receive(BarbaBuffer* data)
{
	if (IsDisposing()) 
		return; 

	_Connection->ReceiveData(data);
}
