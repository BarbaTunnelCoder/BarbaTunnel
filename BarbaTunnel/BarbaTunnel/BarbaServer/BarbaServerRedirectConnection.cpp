#include "StdAfx.h"
#include "BarbaServerRedirectConnection.h"


BarbaServerRedirectConnection::BarbaServerRedirectConnection(BarbaServerConfigItem* configItem, u_long clientVirtualIp, u_long clientIp, 
	u_short clientPort, u_short tunnelPort)
	: BarbaServerConnection(configItem, clientVirtualIp, clientIp)
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
	return ConfigItem->RealPort;
}

bool BarbaServerRedirectConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		packet.SetSrcPort(this->TunnelPort);
		packet.SetDesIp(this->ClientIp);
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
		packet.SetSrcIp(this->ClientVirtualIp);
		packet.SetDesPort(this->GetRealPort());
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		SetWorkingState(packetBuffer->m_Length, send);
	}

	return true;
}
