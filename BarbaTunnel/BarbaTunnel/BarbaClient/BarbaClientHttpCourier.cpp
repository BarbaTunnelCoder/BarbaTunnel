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

void BarbaClientHttpCourier::Crypt(std::vector<BYTE>* data, bool encrypt)
{
	if (IsDisposing()) 
		return; 

	if (encrypt)
	{
		this->HttpConnection->EncryptData(data);
	}
	else
	{
		this->HttpConnection->DecryptData(data);
	}
}

void BarbaClientHttpCourier::Receive(std::vector<BYTE>* data)
{
	if (IsDisposing()) 
		return; 

	this->HttpConnection->DecryptData(data);
	PacketHelper packet((iphdr_ptr)data->data());
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
	std::vector<BYTE> data(packet->GetIpLen());
	memcpy_s(&data.front(), data.size(), packet->ipHeader, packet->GetIpLen());
	this->HttpConnection->EncryptData(&data);
	this->Send(&data);
}

void BarbaClientHttpCourier::GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, std::vector<BYTE>* fakeFileHeader, bool createNew)
{
	if (IsDisposing()) 
		return; 

	if (!theApp->GetFakeFile(&this->HttpConnection->GetConfigItem()->FakeFileTypes, this->CreateStruct.FakeFileMaxSize, filename, contentType, fileSize, fakeFileHeader, createNew))
		BarbaCourierClient::GetFakeFile(filename, contentType, fileSize, fakeFileHeader, createNew);
}

