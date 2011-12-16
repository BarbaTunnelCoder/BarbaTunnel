#include "StdAfx.h"
#include "BarbaClientHttpConnection.h"


BarbaClientHttpConnection::BarbaClientHttpConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem, u_short tunnelPort)
	: BarbaClientConnection(config, configItem)
	, Courier(config->ServerIp, tunnelPort, 4)
{

}


BarbaClientHttpConnection::~BarbaClientHttpConnection(void)
{
	//this->
}
