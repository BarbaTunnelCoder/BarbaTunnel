#include "StdAfx.h"
#include "BarbaConnectionManager.h"
#include "BarbaApp.h"


BarbaConnectionManager::BarbaConnectionManager(void)
{
	this->LastIntervalTime = GetTickCount();
}

void BarbaConnectionManager::Dispose()
{
	//delete all messages
	BarbaConnection* connection = this->Connections.RemoveHead();
	while(connection!=NULL)
	{
		delete connection;
		connection = this->Connections.RemoveHead();
	}
}

BarbaConnectionManager::~BarbaConnectionManager(void)
{
}

void BarbaConnectionManager::RemoveConnection(BarbaConnection* conn)
{
	BarbaLog(_T("BarbaConnection removed. ID: %u"), conn->GetId());
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

BarbaConnection* BarbaConnectionManager::FindByPacketToProcess(PacketHelper* packet)
{
	//FindByPacketToProcess called frequently, good place to check interval
	DoIntervalCheck();

	//find connection
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
