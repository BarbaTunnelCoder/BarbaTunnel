#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaClientHttpConnection.h"


BarbaClientHttpConnection::BarbaClientHttpConnection(BarbaClientConfig* config) 
	: BarbaClientTcpConnectionBase(config)
{
	_Courier = NULL;
}

BarbaClientHttpConnection::~BarbaClientHttpConnection(void)
{
}

void BarbaClientHttpConnection::Init()
{
	BarbaCourierHttpClient::CreateStrcutHttp* cs = new BarbaCourierHttpClient::CreateStrcutHttp();
	InitHelper(cs);
	cs->HostName = GetConfig()->ServerAddress;
	cs->RequestMode = GetConfig()->HttpRequestMode;
	_Courier = new Courier(this, cs);
	_Courier->Init();
}

//-------------------------------- Courier

BarbaClientHttpConnection::Courier::Courier(BarbaClientHttpConnection* connection, BarbaCourierHttpClient::CreateStrcutHttp* cs)
	: BarbaCourierHttpClient(cs)
	, _Connection(connection)
{
}


BarbaClientHttpConnection::Courier::~Courier(void)
{
}

void BarbaClientHttpConnection::Courier::Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt)
{
	if (IsDisposing()) 
		return; 

	_Connection->CryptData(data, dataSize, index, encrypt);
}

void BarbaClientHttpConnection::Courier::Receive(BarbaBuffer* data)
{
	if (IsDisposing()) 
		return; 

	_Connection->ReceiveData(data);
}

std::tstring BarbaClientHttpConnection::Courier::GetHttpPostTemplate(bool bombardMode)
{
	return bombardMode ? theClientApp->HttpPostTemplateBombard : theClientApp->HttpPostTemplate;
}

std::tstring BarbaClientHttpConnection::Courier::GetHttpGetTemplate(bool bombardMode)
{
	return bombardMode ? theClientApp->HttpGetTemplateBombard : theClientApp->HttpGetTemplate;
}

void BarbaClientHttpConnection::Courier::GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew)
{
	if (IsDisposing()) 
		return; 

	theApp->GetFakeFile(&_Connection->GetConfig()->FakeFileTypes, GetCreateStruct()->MaxTransferSize, filename, contentType, fileSize, fakeFileHeader, createNew);
}
