#include "StdAfx.h"
#include "BarbaServerHttpConnection.h"

BarbaServerHttpConnection::BarbaServerHttpConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, u_long clientIp, u_short tunnelPort, u_long sessionId)
	: BarbaServerConnection(configItem, clientVirtualIp, clientIp)
	, HttpCourier(4)
{
	this->SessionId = sessionId;
	this->TunnelPort = tunnelPort;
}

BarbaServerHttpConnection::~BarbaServerHttpConnection(void)
{
}

bool BarbaServerHttpConnection::ProcessPacket(PacketHelper* /*packet*/, bool /*send*/)
{
	return false;
}

bool BarbaServerHttpConnection::AddSocket(BarbaSocket* Socket, bool isOutgoing)
{
	return this->HttpCourier.AddSocket(Socket, isOutgoing);
}

bool BarbaServerHttpConnection::ShouldProcessPacket(PacketHelper* /*packet*/)
{
	return false;
}
