#include "StdAfx.h"
#include "BarbaServerHttpCourier.h"
#include "BarbaServerHttpConnection.h"
#include "BarbaServerApp.h"


BarbaServerHttpCourier::BarbaServerHttpCourier(BarbaCourierCreateStrcut* cs, BarbaServerHttpConnection* httpConnection)
	: HttpConnection(httpConnection)
	, BarbaCourierServer(cs)
{
}

BarbaServerHttpCourier::~BarbaServerHttpCourier(void)
{
}

void BarbaServerHttpCourier::Crypt(BYTE* data, size_t dataLen, bool encrypt)
{
	if (IsDisposing()) 
		return; 

	if (encrypt)
	{
		this->HttpConnection->EncryptData(data, dataLen);
	}
	else
	{
		this->HttpConnection->DecryptData(data, dataLen);
	}
}

void BarbaServerHttpCourier::SendPacket(PacketHelper* packet)
{
	if (IsDisposing()) 
		return; 

	//encrypt whole packet include header
	BYTE data[MAX_ETHER_FRAME];
	size_t dataCount = packet->GetPacketLen();
	memcpy_s(data, sizeof data, packet->ipHeader, dataCount);
	this->HttpConnection->EncryptData(data, dataCount);
	this->Send(data, dataCount);
}

void BarbaServerHttpCourier::Receive(BYTE* buffer, size_t bufferCount)
{
	if (IsDisposing()) 
		return; 

	this->HttpConnection->DecryptData(buffer, bufferCount);
	PacketHelper packet((iphdr_ptr)buffer);
	if (!packet.IsValidChecksum())
	{
		Log(_T("Error: Invalid packet checksum received! Check your key."));
		return;
	}
	this->HttpConnection->ProcessPacket(&packet, false);
}

void BarbaServerHttpCourier::GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, std::vector<BYTE>* fakeFileHeader, bool createNew)
{
	if (IsDisposing()) 
		return; 

	if (!theApp->GetFakeFile(&this->HttpConnection->GetConfigItem()->FakeFileTypes, this->CreateStruct.FakeFileMaxSize, filename, contentType, fileSize, fakeFileHeader, createNew))
		BarbaCourierServer::GetFakeFile(filename, contentType, fileSize, fakeFileHeader, createNew);
}
