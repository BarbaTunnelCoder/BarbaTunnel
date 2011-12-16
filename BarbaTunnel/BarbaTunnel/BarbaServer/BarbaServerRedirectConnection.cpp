#include "StdAfx.h"
#include "BarbaServerRedirectConnection.h"


BarbaServerRedirectConnection::BarbaServerRedirectConnection(LPCTSTR connectionName, BarbaKey* barbaKey, u_short realPort, BarbaModeEnum mode)
	: BarbaServerConnection(connectionName, barbaKey)
{
	this->Mode = mode;
	this->RealPort = realPort;
}


BarbaServerRedirectConnection::~BarbaServerRedirectConnection(void)
{
}

BarbaModeEnum BarbaServerRedirectConnection::GetMode()
{
	return this->Mode;
}

bool BarbaServerRedirectConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		packet.SetSrcPort(this->ClientTunnelPort);
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
		packet.SetDesPort(this->RealPort);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		SetWorkingState(packetBuffer->m_Length, send);
	}

	return true;
}
