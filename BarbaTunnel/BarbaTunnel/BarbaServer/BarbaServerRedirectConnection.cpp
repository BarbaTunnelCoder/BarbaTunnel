#include "StdAfx.h"
#include "BarbaServerRedirectConnection.h"


BarbaServerRedirectConnection::BarbaServerRedirectConnection(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp, 
	u_short clientPort, u_short tunnelPort)
	: BarbaServerConnection(config, clientVirtualIp, clientIp)
{
	this->ClientPort = clientPort;
	this->TunnelPort = tunnelPort;
}


BarbaServerRedirectConnection::~BarbaServerRedirectConnection(void)
{
}

u_short BarbaServerRedirectConnection::GetTunnelPort()
{
	return this->TunnelPort;
}

u_short BarbaServerRedirectConnection::GetRealPort()
{
	return Config->RealPort;
}

bool BarbaServerRedirectConnection::ShouldProcessPacket(PacketHelper* packet)
{
	return 
		(packet->GetDesIp()==this->GetClientVirtualIp()) || //check outgoing packets
		(packet->GetSrcIp()==this->ClientIp && packet->GetSrcPort()==this->ClientPort && packet->GetDesPort()==this->GetTunnelPort() && packet->ipHeader->ip_p==BarbaMode_GetProtocol(GetMode()) );  //check incoming packets
}

bool BarbaServerRedirectConnection::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		packet->SetSrcPort(this->TunnelPort);
		packet->SetDesIp(this->ClientIp);
		EncryptPacket(packet);
		this->SendPacketToOutbound(packet);
	}
	else
	{
		DecryptPacket(packet);
		if (!packet->IsValidChecksum())
			return true; //don't process packet
		packet->SetSrcIp(this->ClientVirtualIp);
		packet->SetDesPort(this->GetRealPort());
		SendPacketToInbound(packet);
	}

	return true;
}
