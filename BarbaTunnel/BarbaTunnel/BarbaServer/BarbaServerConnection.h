#pragma once
#include "BarbaConnection.h"
#include "BarbaServerConfig.h"
#include "PacketHelper.h"

//BarbaServerConnection
class BarbaServerConnection : public BarbaConnection
{
public:
	explicit BarbaServerConnection(LPCTSTR connectionName, BarbaKey* barbaKey);
	virtual ~BarbaServerConnection(){}

	DWORD ClientVirtualIp;
	DWORD ClientLocalIp;
	DWORD ClientIp;
	u_short ClientPort;
	u_short ClientTunnelPort;
	BYTE ClientEthAddress[ETH_ALEN]; //Ethernet address of received packet; useful when router not exists
	//BarbaServerConfigItem* ConfigItem;
	void ReportNewConnection();
};


