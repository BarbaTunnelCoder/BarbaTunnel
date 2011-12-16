#include "StdAfx.h"
#include "BarbaClientRedirectConnection.h"


BarbaClientRedirectConnection::BarbaClientRedirectConnection(LPCTSTR connectionName, BarbaKey* key, BarbaModeEnum mode)
	: BarbaClientConnection(connectionName, key)
{
	this->Mode = mode;
}

BarbaModeEnum BarbaClientRedirectConnection::GetMode()
{
	return this->Mode;
}

BarbaClientRedirectConnection::~BarbaClientRedirectConnection(void)
{
}

bool BarbaClientRedirectConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		packet.SetDesPort(this->TunnelPort);
		packet.SetSrcPort(this->ClientPort);
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
		packet.SetSrcPort(this->ConfigItem->RealPort);
		packet.SetDesPort(this->OrgClientPort);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		SetWorkingState(packetBuffer->m_Length, send);
	}

	return true;
}
