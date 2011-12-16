#include "StdAfx.h"
#include "BarbaConnectionManager.h"


BarbaConnectionManager::BarbaConnectionManager(void)
{
}


BarbaConnectionManager::~BarbaConnectionManager(void)
{
}

void BarbaConnectionManager::RemoveConnection(BarbaConnection* conn)
{
	this->Connections.Remove(conn);
	delete conn;
}

void BarbaConnectionManager::AddConnection(BarbaConnection* conn)
{
	this->CleanTimeoutConnections();
	conn->ReportNewConnection();
	this->Connections.AddTail(conn);
}

void BarbaConnectionManager::CleanTimeoutConnections()
{
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&this->Connections);
	BarbaConnection** connections = autoLockBuf.GetBuffer();
	for (size_t i=0; i<this->Connections.GetCount(); i++)
	{
		BarbaConnection* conn = connections[i];
		if (GetTickCount()-conn->GetLasNegotiationTime()>BARBA_CONNECTION_TIMEOUT)
		{
			RemoveConnection(conn);
		}
	}
}

BarbaConnection* BarbaConnectionManager::FindByPacketToProcess(PacketHelper* packet)
{
	BarbaConnection* ret = NULL;
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&this->Connections);
	BarbaConnection** connections = (BarbaConnection**)autoLockBuf.GetBuffer();
	for (size_t i=0; i<this->Connections.GetCount() && ret==NULL; i++)
	{

		BarbaConnection* conn = connections[i];
		if (conn->ShouldProcessPacket(packet))
			ret = conn;
	}
	return ret;
}
