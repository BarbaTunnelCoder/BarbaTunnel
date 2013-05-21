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
	cs->HostName = Config->ServerAddress;
	cs->RequestDataKeyName = Config->RequestDataKeyName.data();
	cs->MaxConnections = Config->MaxUserConnections;
	cs->ConnectionTimeout = theApp->ConnectionTimeout;
}

BarbaServerTcpConnectionBase::~BarbaServerTcpConnectionBase(void)
{
	theApp->AddThread(GetCourier()->Delete());
}

bool BarbaServerTcpConnectionBase::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		packet->SetDesIp(this->ClientLocalIp);
		packet->RecalculateChecksum();
		SetWorkingState(packet->GetIpLen(), true);

		//encrypt whole packet include header
		BarbaBuffer data(packet->ipHeader, packet->GetIpLen());
		if (!GetCourier()->IsDisposing())
			GetCourier()->Send(&data);
	}
	else
	{
		//Initialize First Attempt
		if (ClientLocalIp==0)
			ClientLocalIp = packet->GetSrcIp();

		packet->SetSrcIp(this->ClientVirtualIp);
		SendPacketToInbound(packet);
	}

	return true;
}

u_long BarbaServerTcpConnectionBase::GetSessionId()
{
	return SessionId;
}

bool BarbaServerTcpConnectionBase::ShouldProcessPacket(PacketHelper* packet)
{
	//just process outgoing packets
	return packet->GetDesIp()==this->GetClientVirtualIp();
}

void BarbaServerTcpConnectionBase::AddSocket(BarbaSocket* Socket, LPCSTR requestString, LPCTSTR requestData)
{
	GetCourier()->AddSocket(Socket, requestString, requestData);
}
