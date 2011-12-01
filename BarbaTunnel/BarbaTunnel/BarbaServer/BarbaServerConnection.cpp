#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConnection.h"
#include "BarbaCrypt.h"

void BarbaServerConnection::CryptPacket(PacketHelper* packet)
{
	BarbaCrypt::CryptPacket(packet, theServerApp->Config.Key, theServerApp->Config.KeyCount);
}

void BarbaServerConnection::ReportConnection()
{
	TCHAR ip[50];
	PacketHelper::ConvertIpToString(this->ClientIp, ip, _countof(ip));
	TCHAR virtualIp[50];
	PacketHelper::ConvertIpToString(this->ClientVirtualIp, virtualIp, _countof(virtualIp));
	LPCTSTR connectionName = _tcslen(ConfigItem->Name)>0 ? ConfigItem->Name : _T("Connection");
	LPCTSTR mode = BarbaMode_ToString(ConfigItem->Mode);
	BarbaLog(_T("New %s! %s - %s:%d, Virtual IP: %s"), connectionName, ip, mode, this->ClientTunnelPort, virtualIp);
	BarbaNotify(_T("New %s\r\nClient IP: %s\r\nClient Virtual IP: %s\r\nProtocol: %s:%d"), connectionName, ip, virtualIp, mode, this->ClientTunnelPort);
}

bool BarbaServerConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer)
{
	CryptPacket(barbaPacket);
	PacketHelper orgPacket(orgPacketBuffer, 0);
	orgPacket.SetEthHeader(barbaPacket->ethHeader);
	return orgPacket.SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload()) && orgPacket.IsValidChecksum();
}

bool BarbaServerConnection::CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer)
{
	packet->RecalculateChecksum();

	PacketHelper barbaPacket(barbaPacketBuffer, IPPROTO_UDP);
	barbaPacket.SetEthHeader(packet->ethHeader);
	barbaPacket.ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket.ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.ipHeader->ip_off = 0;
	barbaPacket.SetSrcIp(packet->GetSrcIp());
	barbaPacket.SetDesIp(this->ClientIp);
	barbaPacket.SetSrcPort(this->ClientTunnelPort);
	barbaPacket.SetDesPort( this->ClientPort );
	barbaPacket.SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	barbaPacket.SetDesEthAddress(this->ClientEthAddress);
	CryptPacket(&barbaPacket);
	return true;
}

bool BarbaServerConnection::ProcessPacketRedirect(INTERMEDIATE_BUFFER* packetBuffer)
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
	}
	else
	{
		CryptPacket(&packet);
		if (!packet.IsValidChecksum())
			return false;
		packet.SetSrcIp(this->ClientVirtualIp);
		packet.SetDesPort(this->ConfigItem->RealPort);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
	}

	return true;
}

bool BarbaServerConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
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

//This function called when the connection should process the current IP
//In receive mode it should check signature after decrypt
//@return true if it process the packet
bool BarbaServerConnection::ProcessPacketUdpTunnel(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (send)
	{
		//test
		//packet.SetDesIp(this->PeerLocalIp);
		//packet.RecalculateChecksum();
		//packet.SetDesEthAddress(this->PeerEthAddress);
		//return true;
			

		packet.SetDesIp(this->ClientLocalIp);

		//Create Barba packet
		BYTE barbaPacketBuffer[MAX_ETHER_FRAME];
		CreateUdpBarbaPacket(&packet, barbaPacketBuffer);
		PacketHelper barbaPacket(barbaPacketBuffer);

		packet.SetEthPacket(barbaPacket.ethHeader);
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

		//Init First Attempt
		if (this->ClientLocalIp==0)
		{
			this->ClientLocalIp = orgPacket.GetSrcIp();
		}
			
		//prepare for NAT
		orgPacket.SetSrcIp(this->ClientVirtualIp);
		orgPacket.RecalculateChecksum();

		//replace current packet with barba packet
		packet.SetEthPacket(orgPacket.ethHeader);
		packet.RecalculateChecksum();
		packetBuffer->m_Length = packet.GetPacketLen();
		return true;
	}
}

BarbaServerConnection* BarbaServerConnectionManager::CreateConnection(PacketHelper* packet, BarbaServerConfigItem* configItem)
{
	CleanTimeoutConnections();

	BarbaServerConnection* conn = new BarbaServerConnection();
	conn->ConfigItem = configItem;
	memcpy_s(conn->ClientEthAddress, ETH_ALEN, packet->ethHeader->h_source, ETH_ALEN);
	conn->ClientIp = packet->GetSrcIp(); 
	conn->ClientVirtualIp = theServerApp->VirtualIpManager.GetNewIp();
	conn->ClientPort = packet->GetSrcPort();
	conn->ClientTunnelPort = packet->GetDesPort();
	conn->ReportConnection();
	Connections[ConnectionsCount++] = conn;
	return conn;
}

void BarbaServerConnectionManager::RemoveConnection(BarbaServerConnection* conn)
{
	for (size_t i=0; i<ConnectionsCount; i++)
	{
		if (Connections[i]==conn)
		{
			Connections[i]=Connections[ConnectionsCount-1];
			theServerApp->VirtualIpManager.ReleaseIp(conn->ClientVirtualIp);
			delete conn;
			ConnectionsCount--;
			break;
		}
	}
}
