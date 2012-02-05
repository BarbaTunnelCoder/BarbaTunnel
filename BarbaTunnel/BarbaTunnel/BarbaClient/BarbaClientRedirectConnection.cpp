#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaClientRedirectConnection.h"


BarbaClientRedirectConnection::BarbaClientRedirectConnection(BarbaClientConfig* config, u_short clientPort, u_short tunnelPort)
	: BarbaClientConnection(config)
{
	this->ClientPort = clientPort;
	this->TunnelPort = tunnelPort;
}

u_short BarbaClientRedirectConnection::GetTunnelPort()
{
	return this->TunnelPort;
}

u_short BarbaClientRedirectConnection::GetRealPort()
{
	return this->Config->RealPort;
}

BarbaClientRedirectConnection::~BarbaClientRedirectConnection(void)
{
}

bool BarbaClientRedirectConnection::ShouldProcessPacket(PacketHelper* packet)
{
	//check outgoing packets
	if (packet->GetDesIp()==GetServerIp())
	{
		return packet->GetSrcPort()==this->ClientPort && BarbaClientApp::ShouldGrabPacket(packet, Config);
	}
	//check incoming packets
	else if (packet->GetSrcIp()==GetServerIp())
	{
		return packet->ipHeader->ip_p==BarbaMode_GetProtocol(GetMode())
			&& packet->GetSrcPort()==this->GetTunnelPort()
			&& packet->GetDesPort()==this->ClientPort;
	}
	else
	{
		return false;
	}
}


bool BarbaClientRedirectConnection::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		//packet->SetDesPort(this->GetTunnelPort());
		//EncryptPacket(packet);
		SendPacketToOutbound(packet);
		return true;
	}
	else
	{
		//DecryptPacket(packet);
		//if (!packet->IsValidChecksum())
			//return true;
		//packet->SetSrcPort(this->GetRealPort());
		SendPacketToInbound(packet);
	}

	return true;
}
