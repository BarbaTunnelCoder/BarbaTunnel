#include "StdAfx.h"
#include "BarbaServerTcpConnectionBase.h"
#include "BarbaServerApp.h"

BarbaServerTcpConnectionBase::BarbaServerTcpConnectionBase(BarbaServerConfig* config, u_long clientVirtualIp, u_long clientIp)
	: BarbaServerConnection(config, clientVirtualIp, clientIp)
{
	ClientLocalIp = 0;
}

void BarbaServerTcpConnectionBase::InitHelper(BarbaCourierTcpServer::CreateStrcutTcp* cs, LPCTSTR requestData)
{
	SessionId = (u_long)BarbaUtils::GetKeyValueFromString(requestData, _T("SessionId"), 0);
	cs->MaxConnections = GetConfig()->MaxUserConnections;
	cs->ConnectionTimeout = theApp->ConnectionTimeout;
}

BarbaServerTcpConnectionBase::~BarbaServerTcpConnectionBase(void)
{
	theApp->AddThread(GetCourier()->Delete());
}

bool BarbaServerTcpConnectionBase::ProcessOutboundPacket(PacketHelper* packet) 
{
	packet->SetDesIp(this->ClientLocalIp);
	packet->RecalculateChecksum();
	SetWorkingState(packet->GetIpLen(), true);

	//encrypt whole packet include header
	BarbaBuffer data(packet->ipHeader, packet->GetIpLen());
	if (!GetCourier()->IsDisposing())
		GetCourier()->Send(&data);
	return true;
}

bool BarbaServerTcpConnectionBase::ProcessInboundPacket(PacketHelper* packet) 
{
	UNREFERENCED_PARAMETER(packet);
	return false;
}

void BarbaServerTcpConnectionBase::ReceiveData(BarbaBuffer* data)
{
	PacketHelper packet((iphdr_ptr)data->data(), data->size());
	if (!packet.IsValidChecksum())
		return;

	ClientLocalIp = packet.GetSrcIp();
	packet.SetSrcIp(ClientVirtualIp);
	SendPacketToInbound(&packet);
}

u_long BarbaServerTcpConnectionBase::GetSessionId()
{
	return SessionId;
}

void BarbaServerTcpConnectionBase::AddSocket(BarbaSocket* Socket, LPCSTR requestString, LPCTSTR requestData)
{
	GetCourier()->AddSocket(Socket, requestString, requestData);
}
