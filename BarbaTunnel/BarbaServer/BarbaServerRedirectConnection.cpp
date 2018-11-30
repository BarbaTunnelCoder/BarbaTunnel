#include "StdAfx.h"
#include "BarbaServerRedirectConnection.h"


BarbaServerRedirectConnection::BarbaServerRedirectConnection(BarbaServerConfig* config, u_long clientVirtualIp, PacketHelper* initPacket)
	: BarbaServerConnection(config, clientVirtualIp, initPacket->GetSrcIp())
{
	ClientPort = initPacket->GetSrcPort();
	TunnelPort = initPacket->GetDesPort();
}

u_long BarbaServerRedirectConnection::GetSessionId()
{
	return ClientPort;
}

BarbaServerRedirectConnection::~BarbaServerRedirectConnection(void)
{
}

u_short BarbaServerRedirectConnection::GetRealPort()
{
	return GetConfig()->RealPort;
}

bool BarbaServerRedirectConnection::ProcessOutboundPacket(PacketHelper* packet)
{
	packet->SetSrcPort(TunnelPort);
	packet->SetDesIp(ClientIp);
	EncryptPacket(packet);
	SendPacketToOutbound(packet);
	Log3(_T("Sending packet with %d bytes."), packet->GetPacketLen());
	return true;
}

bool BarbaServerRedirectConnection::ProcessInboundPacket(PacketHelper* packet)
{
	if (packet->GetSrcPort()!=ClientPort)
		return false;

	DecryptPacket(packet);
	packet->SetSrcIp(ClientVirtualIp);
	packet->SetDesPort(GetRealPort());
	SendPacketToInbound(packet);
	Log3(_T("Receiving packet with %d bytes."), packet->GetPacketLen());
	return true;
}
