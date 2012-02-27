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

void BarbaServerHttpCourier::Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt)
{
	if (IsDisposing()) 
		return; 
	this->HttpConnection->CryptData(data, dataSize, index, encrypt);
}

void BarbaServerHttpCourier::SendPacket(PacketHelper* packet)
{
	if (IsDisposing()) 
		return; 

	//encrypt whole packet include header
	BarbaBuffer data(packet->ipHeader, packet->GetIpLen());
	this->Send(&data);
}

void BarbaServerHttpCourier::Receive(BarbaBuffer* data)
{
	if (IsDisposing()) 
		return; 

	PacketHelper packet((iphdr_ptr)data->data(), data->size());
	if (!packet.IsValidChecksum())
	{
		Log(_T("Error: Invalid packet checksum received! Check your key."));
		return;
	}
	this->HttpConnection->ProcessPacket(&packet, false);
}

void BarbaServerHttpCourier::GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew)
{
	if (IsDisposing()) 
		return; 

	if (!theApp->GetFakeFile(&this->HttpConnection->GetConfigItem()->FakeFileTypes, this->CreateStruct.FakeFileMaxSize, filename, contentType, fileSize, fakeFileHeader, createNew))
		BarbaCourierServer::GetFakeFile(filename, contentType, fileSize, fakeFileHeader, createNew);
}
