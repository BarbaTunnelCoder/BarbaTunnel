#include "StdAfx.h"
#include "BarbaClientTcpConnection.h"
#include "BarbaClientApp.h"


BarbaClientTcpConnection::BarbaClientTcpConnection(BarbaClientConfig* config) 
	: BarbaClientTcpConnectionBase(config)
{
}

BarbaClientTcpConnection::~BarbaClientTcpConnection(void)
{
}

void BarbaClientTcpConnection::Init()
{
	BarbaCourierTcpClient::CreateStrcutTcp* cs = new BarbaCourierTcpClient::CreateStrcutTcp();
	InitHelper(cs);
	_Courier = new Courier(cs, this);
	_Courier->Init();
}

BarbaClientTcpConnection::Courier::Courier(BarbaCourierTcpClient::CreateStrcutTcp* cs, BarbaClientTcpConnection* connection)
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

	PacketHelper packet((iphdr_ptr)data->data(), data->size());
	if (!packet.IsValidChecksum())
	{
		Log2(_T("Error: Invalid packet checksum received! Check your key and version."));
		return;
	}
	_Connection->ProcessPacket(&packet, false);
}
