#include "StdAfx.h"
#include "BarbaServerHttpConnection.h"
#include "BarbaServerApp.h"

BarbaServerHttpConnection::BarbaServerHttpConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp)
	: BarbaServerTcpConnectionBase(config, clientVirtualIp, clientIp)
{
	_Courier = NULL;
}

void BarbaServerHttpConnection::Init(LPCTSTR requestData)
{
	BarbaCourierHttpServer::CreateStrcutHttp* cs = new BarbaCourierHttpServer::CreateStrcutHttp();
	InitHelper(cs, requestData);
	_Courier = new Courier(cs , this);
	_Courier->Init(requestData);
}

BarbaServerHttpConnection::~BarbaServerHttpConnection(void)
{
}

//--------------------------- Courier
BarbaServerHttpConnection::Courier::Courier(CreateStrcutHttp* cs, BarbaServerHttpConnection* connection)
	: BarbaCourierHttpServer(cs)
	, _Connection(connection)
{
}

BarbaServerHttpConnection::Courier::~Courier(void)
{
}

void BarbaServerHttpConnection::Courier::Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt)
{
	if (IsDisposing()) 
		return; 

	_Connection->CryptData(data, dataSize, index, encrypt);
}

void BarbaServerHttpConnection::Courier::Receive(BarbaBuffer* data)
{
	if (IsDisposing()) 
		return;

	_Connection->ReceiveData(data);
}

void BarbaServerHttpConnection::Courier::GetFakeFile(TCHAR* filename, std::tstring* contentType, BarbaBuffer* fakeFileHeader)
{
	if (IsDisposing()) 
		return; 

	theApp->GetFakeFile(NULL, 0, filename, contentType, 0, fakeFileHeader, false);
}

std::tstring BarbaServerHttpConnection::Courier::GetHttpPostReplyRequest(bool bombardMode)
{
	return bombardMode ? theServerApp->HttpPostReplyTemplateBombard : theServerApp->HttpPostReplyTemplate;
}

std::tstring BarbaServerHttpConnection::Courier::GetHttpGetReplyRequest(bool bombardMode)
{
	return bombardMode ? theServerApp->HttpGetReplyTemplateBombard : theServerApp->HttpGetReplyTemplate;
}
