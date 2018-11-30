#include "StdAfx.h"
#include "BarbaClientTcpConnectionBase.h"
#include "BarbaClientApp.h"


BarbaClientTcpConnectionBase::BarbaClientTcpConnectionBase(BarbaClientConfig* config) 
	: BarbaClientConnection(config)
{
	SessionId = BarbaUtils::GetRandom(100000, UINT_MAX-1);
	_Courier = NULL;
}

void BarbaClientTcpConnectionBase::InitHelper(BarbaCourierTcpClient::CreateStrcutTcp* cs)
{
	cs->MaxTransferSize = GetConfig()->MaxTransferSize;
	cs->MaxConnections = GetConfig()->MaxUserConnections;
	cs->SessionId = SessionId;
	cs->ConnectionTimeout = theApp->ConnectionTimeout;
	cs->MinPacketSize = GetConfig()->MinPacketSize;
	cs->KeepAliveInterval = GetConfig()->KeepAliveInterval;
	cs->RemoteIp = GetConfig()->ServerIp;
	cs->PortRange = &GetConfig()->TunnelPorts;
}

BarbaClientTcpConnectionBase::~BarbaClientTcpConnectionBase(void)
{
	theApp->AddThread(GetCourier()->Delete());
}

bool BarbaClientTcpConnectionBase::ProcessOutboundPacket(PacketHelper* packet)
{
	SetWorkingState(packet->GetIpLen(), true);
	
	//encrypt whole packet include header
	BarbaBuffer data(packet->ipHeader, packet->GetIpLen());
	if (!GetCourier()->IsDisposing())
		GetCourier()->Send(&data);
	return true;
}

bool BarbaClientTcpConnectionBase::ProcessInboundPacket(PacketHelper* packet)
{
	UNREFERENCED_PARAMETER(packet);
	return false;
}

void BarbaClientTcpConnectionBase::ReceiveData(BarbaBuffer* data)
{
	PacketHelper packet((iphdr_ptr)data->data(), data->size());
	if (!packet.IsValidChecksum())
	{
		BarbaLog(_T("Error: Invalid packet checksum received!"));
		return;
	}
	SendPacketToInbound(&packet);
}
