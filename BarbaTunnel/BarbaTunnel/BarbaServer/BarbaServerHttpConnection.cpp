#include "StdAfx.h"
#include "BarbaServerHttpConnection.h"
#include "BarbaServerApp.h"

BarbaServerHttpConnection::BarbaServerHttpConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, u_long clientIp, u_short tunnelPort, u_long sessionId)
	: BarbaServerConnection(configItem, clientVirtualIp, clientIp)
{
	this->SessionId = sessionId;
	this->TunnelPort = tunnelPort;
	this->Courier = new BarbaServerHttpCourier(4);
	this->Courier->InitFakeRequests(theServerApp->FakeHttpGetReplyTemplate.data(), theServerApp->FakeHttpPostReplyTemplate.data());
}

BarbaServerHttpConnection::~BarbaServerHttpConnection(void)
{
	theApp->AddThread(this->Courier->Delete());
}

bool BarbaServerHttpConnection::ProcessPacket(PacketHelper* /*packet*/, bool /*send*/)
{
	return false;
}

bool BarbaServerHttpConnection::AddSocket(BarbaSocket* Socket, bool isOutgoing)
{
	return this->Courier->AddSocket(Socket, isOutgoing);
}

bool BarbaServerHttpConnection::ShouldProcessPacket(PacketHelper* /*packet*/)
{
	return false;
}
