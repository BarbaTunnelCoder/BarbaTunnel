#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaClientHttpConnection.h"


BarbaClientHttpConnection::BarbaClientHttpConnection(BarbaClientConfig* config) 
	: BarbaClientTcpConnectionBase(config)
{
}

BarbaClientHttpConnection::~BarbaClientHttpConnection(void)
{
}

void BarbaClientHttpConnection::Init()
{
	BarbaCourierHttpClient::CreateStrcutHttp* cs = new BarbaCourierHttpClient::CreateStrcutHttp();
	InitHelper(cs);
	cs->RequestMode = Config->HttpRequestMode;
	_Courier = new Courier(cs, this);
	_Courier->Init();
}

//-------------------------------- Courier

BarbaClientHttpConnection::Courier::Courier(BarbaCourierHttpClient::CreateStrcutHttp* cs, BarbaClientHttpConnection* connection)
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

	PacketHelper packet((iphdr_ptr)data->data(), data->size());
	if (!packet.IsValidChecksum())
	{
		Log2(_T("Error: Invalid packet checksum received! Check your key and version."));
		return;
	}
	_Connection->ProcessPacket(&packet, false);
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

	theApp->GetFakeFile(&_Connection->GetConfigItem()->FakeFileTypes, GetCreateStruct()->MaxTransferSize, filename, contentType, fileSize, fakeFileHeader, createNew);
}
