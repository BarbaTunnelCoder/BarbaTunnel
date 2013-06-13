#include "stdafx.h"
#include "BarbaClientUdpConnection.h"
#include "BarbaClientApp.h"


BarbaClientUdpConnection::BarbaClientUdpConnection(BarbaClientConfig* config)
	: BarbaClientConnection(config)
{
	_Courier = NULL;
}


BarbaClientUdpConnection::~BarbaClientUdpConnection(void)
{
}

void BarbaClientUdpConnection::Init()
{
	BarbaCourierUdpClient::CreateStrcutUdp* cs = new BarbaCourierUdpClient::CreateStrcutUdp();
	cs->MaxChunkSize = GetConfig()->MaxPacketSize - sizeof (iphdr) - sizeof (udphdr);
	cs->KeepAliveInterval = GetConfig()->KeepAliveInterval;
	cs->RemoteIp = GetConfig()->ServerIp;
	cs->PortRange = &GetConfig()->TunnelPorts;
	_Courier = new Courier(this, cs);
	_Courier->Init();
}

bool BarbaClientUdpConnection::ProcessOutboundPacket(PacketHelper* packet)
{
	memcpy_s(&LastOutboundIpHeader, sizeof iphdr, packet->ipHeader, sizeof iphdr);
	BarbaBuffer buffer((BYTE*)packet->ipHeader, packet->GetIpLen());
	Log3(_T("Sending packet with %d bytes."), packet->GetIpLen());
	GetCourier()->SendData(&buffer);
	return true;
}

bool BarbaClientUdpConnection::ProcessInboundPacket(PacketHelper* packet)
{
	//process by courier
	return GetCourier()->ProcessInboundPacket(packet);

}

u_long BarbaClientUdpConnection::GetSessionId()
{
	return GetCourier()->GetSessionId();
}

//Courier Implementation
BarbaClientUdpConnection::Courier::Courier(BarbaClientUdpConnection* connection, CreateStrcutUdp* cs)
	: BarbaCourierUdpClient(cs)
{
	_Connection = connection;
}

void BarbaClientUdpConnection::Courier::ReceiveData(BarbaBuffer* data)
{
	PacketHelper orgPacket;
	orgPacket.SetIpPacket((iphdr_ptr)data->data(), data->size());
	if (!orgPacket.IsValidChecksum())
	{
		Log2(_T("Error: Dropping assembled packet with invalid checksum!"));
		return;
	}

	Log3(_T("Receving packet with %d bytes."), orgPacket.GetIpLen());
	_Connection->SendPacketToInbound(&orgPacket);
}

void BarbaClientUdpConnection::Courier::SendUdpPacketToOutbound(DWORD remoteIp, u_short srcPort, u_short desPort, BarbaBuffer* payLoad)
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

void BarbaClientUdpConnection::Courier::Encrypt(BYTE* data, size_t dataSize, size_t index)
{
	_Connection->CryptData(data, dataSize, index, true);
}

void BarbaClientUdpConnection::Courier::Decrypt(BYTE* data, size_t dataSize, size_t index)
{
	_Connection->CryptData(data, dataSize, index, false);
}

