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

void BarbaClientHttpCourier::Crypt(BYTE* data, size_t dataLen, bool encrypt)
{
	if (encrypt)
	{
		this->HttpConnection->EncryptData(data, dataLen);
	}
	else
	{
		this->HttpConnection->DecryptData(data, dataLen);
	}
}

void BarbaClientHttpCourier::Receive(BYTE* buffer, size_t bufferCount)
{
	this->HttpConnection->DecryptData(buffer, bufferCount);
	PacketHelper packet;
	if (!packet.SetIpPacket((iphdr_ptr)buffer) || !packet.IsValidChecksum())
	{
		Log(_T("Error: Invalid packet checksum received! Check your key."));
	}
	this->HttpConnection->ProcessPacket(&packet, false);
}

void BarbaClientHttpCourier::SendPacket(PacketHelper* packet)
{
	//encrypt whole packet include header
	BYTE data[MAX_ETHER_FRAME];
	size_t dataCount = packet->GetPacketLen();
	memcpy_s(data, sizeof data, packet->ipHeader, dataCount);
	this->HttpConnection->EncryptData(data, dataCount);
	this->Send(data, dataCount);
}

void BarbaClientHttpCourier::GetFakeFile(TCHAR* filename, u_int* fileSize, std::vector<BYTE>* fakeFileHeader, bool createNew)
{
	if (!theApp->GetFakeFile(&this->HttpConnection->GetConfigItem()->FakeFileTypes, this->FakeFileMaxSize, filename, fileSize, fakeFileHeader, createNew))
		BarbaCourierClient::GetFakeFile(filename, fileSize, fakeFileHeader, createNew);
}

