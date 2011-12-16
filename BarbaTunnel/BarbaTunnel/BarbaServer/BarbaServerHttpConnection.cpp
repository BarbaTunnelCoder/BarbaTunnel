#include "StdAfx.h"
#include "BarbaServerHttpConnection.h"

BarbaServerHttpConnection::BarbaServerHttpConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, u_short clientIp, u_long sessionId)
	: BarbaServerConnection(configItem, clientVirtualIp, clientIp)
	, HttpCourier(4)
{
	this->SessionId = sessionId;
}

BarbaServerHttpConnection::~BarbaServerHttpConnection(void)
{
}

bool BarbaServerHttpConnection::ProcessPacket(INTERMEDIATE_BUFFER* /*packetBuffer*/)
{
	//bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	//PacketHelper packet(packetBuffer->m_IBuffer);

	return false;
}

u_long BarbaServerHttpConnection::GetSessionId()
{
	return this->SessionId;
}

bool BarbaServerHttpConnection::AddSocket(BarbaSocket* Socket, bool isOutgoing)
{
	return this->HttpCourier.AddSocket(Socket, isOutgoing);
}

bool BarbaServerHttpConnection::ShouldProcessPacket(PacketHelper* /*packet*/)
{
	return false;
}
