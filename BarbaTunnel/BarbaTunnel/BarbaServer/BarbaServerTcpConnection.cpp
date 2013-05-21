#include "StdAfx.h"
#include "BarbaServerTcpConnection.h"
#include "BarbaServerApp.h"

BarbaServerTcpConnection::BarbaServerTcpConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp)
	: BarbaServerTcpConnectionBase(config, clientVirtualIp, clientIp)
{
}

void BarbaServerTcpConnection::Init(LPCTSTR requestData)
{
	BarbaCourierTcpServer::CreateStrcutTcp* cs = new BarbaCourierTcpServer::CreateStrcutTcp();
	InitHelper(cs, requestData);
	_Courier = new Courier(cs , this);
	_Courier->Init(requestData);
}

BarbaServerTcpConnection::~BarbaServerTcpConnection(void)
{
}

//--------------------------- Courier
BarbaServerTcpConnection::Courier::Courier(CreateStrcutTcp* cs, BarbaServerTcpConnection* connection)
	: BarbaCourierTcpServer(cs)
	, _Connection(connection)
{
}

BarbaServerTcpConnection::Courier::~Courier(void)
{
}

void BarbaServerTcpConnection::Courier::Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt)
{
	if (IsDisposing()) 
		return; 

	_Connection->CryptData(data, dataSize, index, encrypt);
}

void BarbaServerTcpConnection::Courier::Receive(BarbaBuffer* data)
{
	if (IsDisposing()) 
		return; 

	PacketHelper packet((iphdr_ptr)data->data(), data->size());
	if (!packet.IsValidChecksum())
	{
		Log2(_T("Error: Invalid packet checksum received! Check your key."));
		return;
	}
	_Connection->ProcessPacket(&packet, false);
}