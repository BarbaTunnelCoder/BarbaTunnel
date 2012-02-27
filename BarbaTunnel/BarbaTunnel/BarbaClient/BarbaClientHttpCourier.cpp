#include "StdAfx.h"
#include "BarbaClientHttpCourier.h"
#include "BarbaClientHttpConnection.h"
#include "BarbaClientApp.h"


BarbaClientHttpCourier::BarbaClientHttpCourier(BarbaCourierCreateStrcut* cs, DWORD remoteIp, u_short remotePort, BarbaClientHttpConnection* httpConnection)
	: HttpConnection(httpConnection)
	, BarbaCourierClient(cs, remoteIp, remotePort)
{
}


BarbaClientHttpCourier::~BarbaClientHttpCourier(void)
{
}

void BarbaClientHttpCourier::Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt)
{
	if (IsDisposing()) 
		return; 
	this->HttpConnection->CryptData(data, dataSize, index, encrypt);
}

void BarbaClientHttpCourier::Receive(BarbaBuffer* data)
{
	if (IsDisposing()) 
		return; 

	PacketHelper packet((iphdr_ptr)data->data(), data->size());
	if (!packet.IsValidChecksum())
	{
		Log(_T("Error: Invalid packet checksum received! Check your key."));
	}
	this->HttpConnection->ProcessPacket(&packet, false);
}

void BarbaClientHttpCourier::SendPacket(PacketHelper* packet)
{
	if (IsDisposing()) 
		return; 

	//encrypt whole packet include header
	BarbaBuffer data(packet->ipHeader, packet->GetIpLen());
	this->Send(&data);
}

void BarbaClientHttpCourier::GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew)
{
	if (IsDisposing()) 
		return; 

	if (!theApp->GetFakeFile(&this->HttpConnection->GetConfigItem()->FakeFileTypes, this->CreateStruct.FakeFileMaxSize, filename, contentType, fileSize, fakeFileHeader, createNew))
		BarbaCourierClient::GetFakeFile(filename, contentType, fileSize, fakeFileHeader, createNew);
}

