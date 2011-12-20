#include "StdAfx.h"
#include "BarbaConnectionManager.h"


BarbaConnectionManager::BarbaConnectionManager(void)
{
}


BarbaConnectionManager::~BarbaConnectionManager(void)
{
	//delete all messages
	BarbaConnection* connection = this->Connections.RemoveHead();
	while(connection!=NULL)
	{
		delete connection;
		connection = this->Connections.RemoveHead();
	}
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
		if (GetTickCount()-conn->GetLasNegotiationTime()>BARBA_ConnectionTimeout)
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
