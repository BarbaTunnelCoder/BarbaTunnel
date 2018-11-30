#include "stdafx.h"
#include "BarbaClientUdpConnection.h"
#include "BarbaClientApp.h"


BarbaClientUdpConnection::BarbaClientUdpConnection(BarbaClientConfig* config, PacketHelper* initPacket)
	: BarbaClientConnection(config)
{
	_Courier = NULL;
	LocalIp = initPacket->GetSrcIp();;
}


BarbaClientUdpConnection::~BarbaClientUdpConnection(void)
{
	delete _Courier;
}

void BarbaClientUdpConnection::Init()
{
	BarbaCourierUdpClient::CreateStrcutUdp* cs = new BarbaCourierUdpClient::CreateStrcutUdp();
	cs->MaxChunkSize = GetConfig()->MaxPacketSize - sizeof (iphdr) - sizeof (udphdr);
	cs->KeepAliveInterval = GetConfig()->KeepAliveInterval;
	cs->KeepAlivePortsCount = GetConfig()->KeepAlivePortsCount;
	cs->RemoteIp = GetConfig()->ServerIp;
	cs->PortRange = &GetConfig()->TunnelPorts;
	_Courier = new Courier(this, cs);
	_Courier->Init();
}

bool BarbaClientUdpConnection::ProcessOutboundPacket(PacketHelper* packet)
{
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

	_Connection->Log3(_T("Receiving packet with %d bytes."), orgPacket.GetIpLen());
	_Connection->SendPacketToInbound(&orgPacket);
}

void BarbaClientUdpConnection::Courier::SendUdpPacketToOutbound(DWORD remoteIp, u_short srcPort, u_short desPort, BarbaBuffer* payLoad)
{
	PacketHelper barbaPacket;
	barbaPacket.Reset(IPPROTO_UDP, sizeof iphdr + sizeof udphdr + payLoad->size());
	barbaPacket.ipHeader->ip_ttl = 128;
	barbaPacket.ipHeader->ip_v = 4;
	barbaPacket.ipHeader->ip_off = 0;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.SetSrcIp(_Connection->LocalIp);
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

