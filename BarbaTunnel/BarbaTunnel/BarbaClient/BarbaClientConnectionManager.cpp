#include "StdAfx.h"
#include "BarbaClientConnectionManager.h"
#include "BarbaClientRedirectConnection.h"
#include "BarbaClientUdpConnection.h"


BarbaClientConnectionManager::BarbaClientConnectionManager(void)
{
}


BarbaClientConnectionManager::~BarbaClientConnectionManager(void)
{
}

BarbaClientConnection* BarbaClientConnectionManager::FindByPacketToProcess(PacketHelper* packet)
{
	BarbaClientConnection* ret = NULL;
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&this->Connections);
	BarbaClientConnection** connections = (BarbaClientConnection**)autoLockBuf.GetBuffer();
	for (size_t i=0; i<this->Connections.GetCount() && ret==NULL; i++)
	{

		BarbaClientConnection* conn = connections[i];
		if (conn->ShouldProcessPacket(packet))
			ret = conn;
	}
	return ret;
}

BarbaClientConnection* BarbaClientConnectionManager::CreateConnection(PacketHelper* packet, BarbaClientConfig* config, BarbaClientConfigItem* configItem)
{
	BarbaClientConnection* conn = NULL;
	if (configItem->Mode==BarbaModeTcpRedirect || configItem->Mode==BarbaModeUdpRedirect)
	{
		conn = new BarbaClientRedirectConnection(config, configItem, packet->GetSrcPort(), configItem->GetNewTunnelPort());
	}
	else if (configItem->Mode==BarbaModeUdpTunnel)
	{
		conn = new BarbaClientUdpConnection(config, configItem, packet->GetSrcPort(), configItem->GetNewTunnelPort());
	}
	else
	{
		throw _T("Unsupported mode!");
	}

	AddConnection(conn);
	return conn;
}
