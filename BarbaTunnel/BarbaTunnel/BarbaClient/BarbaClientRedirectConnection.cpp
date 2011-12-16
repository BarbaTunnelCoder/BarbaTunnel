#include "StdAfx.h"
#include "BarbaClientRedirectConnection.h"


BarbaClientRedirectConnection::BarbaClientRedirectConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem, u_short clientPort, u_short tunnelPort)
	: BarbaClientConnection(config, configItem)
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
	return this->ConfigItem->RealPort;
}

BarbaClientRedirectConnection::~BarbaClientRedirectConnection(void)
{
}

bool BarbaClientRedirectConnection::ShouldProcessPacket(PacketHelper* packet)
{
	//check outgoing packets
	if (packet->GetDesIp()==GetServerIp())
	{
		return packet->GetSrcPort()==this->ClientPort && ConfigItem->IsGrabPacket(packet);
	}
	//check incoming packets
	else if (packet->GetSrcIp()==GetServerIp())
	{
		return packet->ipHeader->ip_p==this->ConfigItem->GetTunnelProtocol() 
			&& packet->GetSrcPort()==this->GetTunnelPort()
			&&	packet->GetDesPort()==this->ClientPort;
	}
	else
	{
		return false;
	}
}


bool BarbaClientRedirectConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		packet.SetDesPort(this->GetTunnelPort());
		CryptPacket(&packet);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		SetWorkingState(packetBuffer->m_Length, send);
	}
	else
	{
		CryptPacket(&packet);
		if (!packet.IsValidChecksum())
			return false;
		packet.SetSrcPort(this->GetRealPort());
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		SetWorkingState(packetBuffer->m_Length, send);
	}

	return true;
}
