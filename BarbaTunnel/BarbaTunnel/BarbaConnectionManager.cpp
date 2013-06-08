#include "StdAfx.h"
#include "BarbaConnectionManager.h"
#include "BarbaApp.h"


BarbaConnectionManager::BarbaConnectionManager(void)
{
	LastIntervalTime = GetTickCount();
}

void BarbaConnectionManager::Dispose()
{
	//delete all messages
	BarbaConnection* connection = Connections.RemoveHead();
	while(connection!=NULL)
	{
		delete connection;
		connection = Connections.RemoveHead();
	}
}

BarbaConnectionManager::~BarbaConnectionManager(void)
{
}

void BarbaConnectionManager::RemoveConnection(BarbaConnection* conn)
{
	BarbaLog(_T("BarbaConnection removed. ID: %u"), conn->GetId());
	Connections.Remove(conn);
	delete conn;
}

void BarbaConnectionManager::AddConnection(BarbaConnection* conn)
{
	CleanTimeoutConnections();
	conn->ReportNewConnection();
	Connections.AddTail(conn);
}

void BarbaConnectionManager::CleanTimeoutConnections()
{
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&Connections);
	BarbaConnection** connections = autoLockBuf.GetBuffer();
	for (size_t i=0; i<Connections.GetCount(); i++)
	{
		BarbaConnection* conn = connections[i];
		if (GetTickCount()-conn->GetLasNegotiationTime()>theApp->ConnectionTimeout)
		{
			RemoveConnection(conn);
		}
	}
}

void BarbaConnectionManager::DoIntervalCheck()
{
	//check timeout connections
	if (GetTickCount()<LastIntervalTime+5000)
		return;
	
	LastIntervalTime = GetTickCount();
	CleanTimeoutConnections();
}

BarbaConnection* BarbaConnectionManager::FindByConfig(BarbaConfig* config)
{
	//FindByPacketToProcess called frequently, good place to check interval
	DoIntervalCheck();

	//find connection
	SimpleSafeList<BarbaConnection*>::AutoLockBuffer autoLockBuf(&Connections);
	BarbaConnection** connections = (BarbaConnection**)autoLockBuf.GetBuffer();
	for (int i=0; i<Connections.GetCount(); i++)
	{
		BarbaConnection* conn = connections[i];
		if (conn->GetConfig()==config)
			return conn;
	}
	return NULL;
}
