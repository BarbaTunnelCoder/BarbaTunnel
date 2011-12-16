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
	BarbaConnection** connections = this->Connections.LockBuffer();
	for (size_t i=0; i<this->Connections.GetCount(); i++)
	{
		BarbaConnection* conn = connections[i];
		if (GetTickCount()-conn->GetLasNegotiationTime()>BARBA_CONNECTION_TIMEOUT)
		{
			RemoveConnection(conn);
		}
	}
}
