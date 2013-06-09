#include "stdafx.h"
#include "BarbaClientUdpConnection.h"
#include "BarbaClientApp.h"


BarbaClientUdpConnection::BarbaClientUdpConnection(BarbaClientConfig* config)
	: BarbaClientConnection(config)
{
}


BarbaClientUdpConnection::~BarbaClientUdpConnection(void)
{
}

bool BarbaClientUdpConnection::ProcessOutboundPacket(PacketHelper* packet)
{
	BarbaBuffer buffer((BYTE*)packet->ipHeader, packet->GetIpLen());
	GetCourier()->SendData(&buffer);
	return true;
}

bool BarbaClientUdpConnection::ProcessInboundPacket(PacketHelper* packet)
{
	//store last LastEtherHeader
	memcpy_s(packet->ethHeader, sizeof packet->ethHeader, &LastEtherHeader, sizeof LastEtherHeader);
	
	//process by courier
	return GetCourier()->ProcessInboundPacket(packet);

}

//Courier Implementation
BarbaClientUdpConnection::Courier::Courier(BarbaClientUdpConnection* connection, CreateStrcutUdp* cs)
	: BarbaCourierUdpClient(cs)
{
	_Connection = connection;
}

void BarbaClientUdpConnection::Courier::ReceiveData(BarbaBuffer* data)
{
	PacketHelper orgPacket;
	orgPacket.SetEthHeader(&_Connection->LastEtherHeader);
	orgPacket.SetIpPacket((iphdr_ptr)data->data(), data->size());
	if (orgPacket.IsValidChecksum())
		_Connection->SendPacketToInbound(&orgPacket);
}

void BarbaClientUdpConnection::Courier::SendPacketToOutbound(PacketHelper* packet)
{
	_Connection->SendPacketToOutbound(packet);
}

void BarbaClientUdpConnection::Courier::Encrypt(BYTE* data, size_t dataSize, size_t index)
{
	_Connection->CryptData(data, dataSize, index, true);
}

void BarbaClientUdpConnection::Courier::Decrypt(BYTE* data, size_t dataSize, size_t index)
{
	_Connection->CryptData(data, dataSize, index, false);
}

