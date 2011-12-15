#include "StdAfx.h"
#include "BarbaServerHttpConnection.h"

BarbaServerHttpConnection::BarbaServerHttpConnection(u_long sessionId)
	: HttpCourier(4)
{
	this->SessionId = sessionId;
}

BarbaServerHttpConnection::~BarbaServerHttpConnection(void)
{
}

bool BarbaServerHttpConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	return false;
}

BarbaModeEnum BarbaServerHttpConnection::GetMode()
{
	return BarbaModeEnum::BarbaModeHttpTunnel;
}

u_long BarbaServerHttpConnection::GetSessionId()
{
	return this->SessionId;
}

bool BarbaServerHttpConnection::AddSocket(BarbaSocket* Socket, bool isOutgoing)
{
	return this->HttpCourier.AddSocket(Socket, isOutgoing);
}
