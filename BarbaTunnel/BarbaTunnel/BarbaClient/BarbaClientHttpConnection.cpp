#include "StdAfx.h"
#include "BarbaClientHttpConnection.h"


BarbaClientHttpConnection::BarbaClientHttpConnection(LPCTSTR connectionName, BarbaKey* key, DWORD remoteIp, u_short remotePort, u_short maxConnenction)
	: BarbaClientConnection(connectionName, key)
	, Courier(remoteIp, remotePort, maxConnenction)
{

}


BarbaClientHttpConnection::~BarbaClientHttpConnection(void)
{
	//this->
}
