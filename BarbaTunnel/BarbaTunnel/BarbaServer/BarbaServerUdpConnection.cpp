#include "stdafx.h"
#include "BarbaServerUdpConnection.h"
#include "BarbaServerApp.h"


BarbaServerUdpConnection::BarbaServerUdpConnection(BarbaServerConfig* config, u_long clientVirtualIp, PacketHelper* initPacket)
	: BarbaServerConnection(config, clientVirtualIp, initPacket->GetSrcIp())
{
	ClientLocalIp = 0;
	_Courier = NULL;
}

BarbaServerUdpConnection::~BarbaServerUdpConnection(void)
{
}

DWORD BarbaServerUdpConnection::GetSessionId()
{
	return _Courier->GetSessionId();
}

void BarbaServerUdpConnection::Init()
{
	BarbaCourierUdpServer::CreateStrcutUdp* cs = new BarbaCourierUdpServer::CreateStrcutUdp();
	cs->RemoteIp = ClientIp;
	_Courier = new Courier(this, cs);
	_Courier->Init();
}

bool BarbaServerUdpConnection::ProcessOutboundPacket(PacketHelper* packet)
{
	memcpy_s(&LastOutboundIpHeader, sizeof iphdr, packet->ipHeader, sizeof iphdr);
	packet->SetDesIp(ClientLocalIp);
	packet->RecalculateChecksum();
	
	BarbaBuffer buffer((BYTE*)packet->ipHeader, packet->GetIpLen());
	Log3(_T("Sending packet with %d bytes."), buffer.size());
	GetCourier()->SendData(&buffer);
	return true;
}

bool BarbaServerUdpConnection::ProcessInboundPacket(PacketHelper* packet)
{
	//process by courier
	return GetCourier()->ProcessInboundPacket(packet);

}

//Courier Implementation
BarbaServerUdpConnection::Courier::Courier(BarbaServerUdpConnection* connection, CreateStrcutUdp* cs)
	: BarbaCourierUdpServer(cs)
{
	_Connection = connection;
}

void BarbaServerUdpConnection::Courier::ReceiveData(BarbaBuffer* data)
{
	PacketHelper orgPacket;
	orgPacket.SetIpPacket((iphdr_ptr)data->data(), data->size());
	if (!orgPacket.IsValidChecksum())
	{
		Log2(_T("Error: Dropping assembled packet with invalid checksum!"));
		return;
	}

	//Initialize First Attempt
	_Connection->ClientLocalIp = orgPacket.GetSrcIp();

	//prepare for NAT
	orgPacket.SetSrcIp(_Connection->ClientVirtualIp);

	_Connection->Log3(_T("Receiving packet with %d bytes."), orgPacket.GetIpLen());
	BarbaLog("ssssssss %s to %s,  %d", BarbaUtils::ConvertIpToString(orgPacket.GetSrcIp(), false).data(), BarbaUtils::ConvertIpToString(orgPacket.GetDesIp(), false).data(), orgPacket.ipHeader->ip_sum );
	_Connection->SendPacketToInbound(&orgPacket);
}

void BarbaServerUdpConnection::Courier::SendUdpPacketToOutbound(DWORD remoteIp, u_short srcPort, u_short desPort, BarbaBuffer* payLoad)
{
	PacketHelper barbaPacket;
	barbaPacket.Reset(IPPROTO_UDP, sizeof iphdr + sizeof udphdr + payLoad->size());
	barbaPacket.ipHeader->ip_ttl = _Connection->LastOutboundIpHeader.ip_ttl;
	barbaPacket.ipHeader->ip_v = _Connection->LastOutboundIpHeader.ip_v;
	barbaPacket.ipHeader->ip_off = _Connection->LastOutboundIpHeader.ip_off;
	barbaPacket.ipHeader->ip_src = _Connection->LastOutboundIpHeader.ip_src;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.SetDesIp(remoteIp);
	barbaPacket.SetSrcPort(srcPort);
	barbaPacket.SetDesPort(desPort);
	barbaPacket.SetUdpPayload(payLoad->data(), payLoad->size());
	_Connection->SendPacketToOutbound(&barbaPacket);
}

void BarbaServerUdpConnection::Courier::Encrypt(BYTE* data, size_t dataSize, size_t index)
{
	_Connection->CryptData(data, dataSize, index, true);
}

void BarbaServerUdpConnection::Courier::Decrypt(BYTE* data, size_t dataSize, size_t index)
{
	_Connection->CryptData(data, dataSize, index, false);
}

