#include "StdAfx.h"
#include "BarbaClientApp.h"
#include "BarbaCrypt.h"

void BarbaClientConnection::CryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, Config->Key, Config->KeyCount);
}

void BarbaClientConnection::ReportConnection()
{
	TCHAR serverIp[100];
	PacketHelper::ConvertIpToString(Config->ServerIp, serverIp, _countof(serverIp));
	LPCTSTR mode = BarbaMode_ToString(ConfigItem->Mode);
	LPCTSTR connectionName = _tcslen(ConfigItem->Name)>0 ? ConfigItem->Name : _T("Connection");
	TCHAR serverName[BARBA_MAX_CONFIGNAME];
	if (_tcslen(Config->ServerName)>0)
		_stprintf_s(serverName, _T("%s (%s)"), Config->ServerName, serverIp);
	else
		_stprintf_s(serverName, _T("%s"), serverIp);
	BarbaLog(_T("New %s, Server: %s, Protocol: %s:%d"), connectionName, serverName, mode, this->TunnelPort);
	BarbaNotify(_T("New %s\r\nServer: %s\r\nProtocol: %s:%d"), connectionName, serverName, mode, this->TunnelPort);
}

bool BarbaClientConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer)
{
	CryptPacket(barbaPacket);
	PacketHelper orgPacket(orgPacketBuffer, 0);
	orgPacket.SetEthHeader(barbaPacket->ethHeader);
	return orgPacket.SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload()) && orgPacket.IsValidChecksum();
}

bool BarbaClientConnection::CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer)
{
	packet->RecalculateChecksum();

	PacketHelper barbaPacket(barbaPacketBuffer, IPPROTO_UDP);
	barbaPacket.SetEthHeader(packet->ethHeader);
	barbaPacket.ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket.ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.ipHeader->ip_off = packet->ipHeader->ip_off;
	barbaPacket.SetSrcIp(packet->GetSrcIp());
	barbaPacket.SetDesIp(this->Config->ServerIp);
	barbaPacket.SetSrcPort(this->ClientPort);
	barbaPacket.SetDesPort(this->TunnelPort);
	barbaPacket.SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	CryptPacket(&barbaPacket);
	return true;
}

//This function called when the connection should process the current IP
//In receive mode it should check signature after decrypt
//@return true if it process the packet
bool BarbaClientConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool ret = false;
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;

	switch(ConfigItem->Mode){
	case BarbaModeTcpRedirect:
	case BarbaModeUdpRedirect:
		ret = ProcessPacketRedirect(packetBuffer);
		break;
	
	case BarbaModeUdpTunnel:
		ret = ProcessPacketUdpTunnel(packetBuffer);
		break;
	}

	if (ret)
	{
		this->LasNegotiationTime = GetTickCount();
		theApp->Comm.SetWorkingState(packetBuffer->m_Length, send);
	}
	
	return false;
}

bool BarbaClientConnection::ProcessPacketRedirect(INTERMEDIATE_BUFFER* packetBuffer)
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
	}

	return true;
}

bool BarbaClientConnection::ProcessPacketUdpTunnel(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		//Create Barba packet
		BYTE barbaPacket[MAX_ETHER_FRAME];
		if (!CreateUdpBarbaPacket(&packet, barbaPacket))
			return false;
			
		//replace current packet with barba packet
		packet.SetEthPacket((ether_header_ptr)barbaPacket);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		return true;
	}
	else
	{
		//extract Barba packet
		BYTE orgPacketBuffer[MAX_ETHER_FRAME];
		if (!ExtractUdpBarbaPacket(&packet, orgPacketBuffer))
			return false;
		PacketHelper orgPacket(orgPacketBuffer);
	
		packet.SetEthPacket(orgPacket.ethHeader);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();

		return true;
	}

}

BarbaClientConnection* BarbaClientConnectionManager::CreateConnection(PacketHelper* packet, BarbaClientConfig* config, BarbaClientConfigItem* configItem)
{
	CleanTimeoutConnections();

	BarbaClientConnection* conn = new BarbaClientConnection();
	conn->Config = config;
	conn->ConfigItem = configItem;
	conn->ClientPort = 1419;
	conn->OrgClientPort = packet->GetSrcPort();
	conn->TunnelPort = conn->ConfigItem->GetNewTunnelPort();
	conn->ReportConnection();
	conn->LasNegotiationTime = GetTickCount();
	Connections[ConnectionsCount++] = conn;
	return conn;
}
