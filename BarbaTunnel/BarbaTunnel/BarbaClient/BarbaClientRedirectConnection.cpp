#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaClientRedirectConnection.h"


BarbaClientRedirectConnection::BarbaClientRedirectConnection(BarbaClientConfig* config, PacketHelper* initPacket)
	: BarbaClientConnection(config)
{
	ClientPort = initPacket->GetSrcPort();
	TunnelPort = config->TunnelPorts.GetRandomPort();
}

u_short BarbaClientRedirectConnection::GetRealPort()
{
	return GetConfig()->GrabProtocols[0].Port;
}

u_long BarbaClientRedirectConnection::GetSessionId()
{
	return ClientPort;
}

BarbaClientRedirectConnection::~BarbaClientRedirectConnection(void)
{
}

bool BarbaClientRedirectConnection::ProcessOutboundPacket(PacketHelper* packet)
{
	if (packet->GetSrcPort()!=ClientPort)
		return false; //it is another UDP connection

	packet->SetDesPort(TunnelPort);
	EncryptPacket(packet);
	SendPacketToOutbound(packet);
	Log3(_T("Sending packet with %d bytes."), packet->GetPacketLen());
	return true;
}

bool BarbaClientRedirectConnection::ProcessInboundPacket(PacketHelper* packet)
{
	if (packet->GetSrcPort()!=TunnelPort || packet->GetDesPort()!=ClientPort)
		return false;

	DecryptPacket(packet);
	packet->SetSrcPort(GetRealPort());
	SendPacketToInbound(packet);
	Log3(_T("Receving packet with %d bytes."), packet->GetPacketLen());
	return true;
}