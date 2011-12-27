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

void BarbaServerHttpCourier::SendPacket(PacketHelper* packet)
{
	//encrypt whole packet include header
	BYTE data[MAX_ETHER_FRAME];
	size_t dataCount = packet->GetPacketLen();
	memcpy_s(data, sizeof data, packet->ipHeader, dataCount);
	this->HttpConnection->EncryptData(data, dataCount);
	this->Send(data, dataCount);
}

void BarbaServerHttpCourier::Receive(BYTE* buffer, size_t bufferCount)
{
	this->HttpConnection->DecryptData(buffer, bufferCount);
	PacketHelper packet;
	if (!packet.SetIpPacket((iphdr_ptr)buffer) || !packet.IsValidChecksum())
	{
		Log(_T("Error: Invalid packet checksum received! Check your key."));
		return;
	}
	this->HttpConnection->ProcessPacket(&packet, false);
}

void BarbaServerHttpCourier::GetFakeFile(TCHAR* filename, u_int* fileSize, std::vector<BYTE>* fakeFileHeader, bool createNew)
{
	if (!theApp->GetFakeFile(&this->HttpConnection->GetConfigItem()->FakeFileTypes, this->FakeFileMaxSize, filename, fileSize, fakeFileHeader, createNew))
		BarbaCourierServer::GetFakeFile(filename, fileSize, fakeFileHeader, createNew);
}
