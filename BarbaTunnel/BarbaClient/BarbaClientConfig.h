#pragma once
#include "BarbaConfig.h"
#include "BarbaCourierRequestMode.h"

//BarbaClientConfig
class BarbaClientConfig : public BarbaConfig
{
public:
	BarbaClientConfig();
	bool LoadFile(LPCTSTR file) override;
	virtual ~BarbaClientConfig(){}
	static void LoadFolder(LPCTSTR folder, BarbaArray<BarbaClientConfig>* configs);
	u_short MinPacketSize;
	u_short MaxPacketSize;
	u_short KeepAlivePortsCount;
	DWORD KeepAliveInterval;
	BarbaArray<std::tstring> FakeFileTypes; //use by HTTP-Tunnel
	u_int MaxTransferSize; //use by HTTP-Tunnel
	BarbaArray<ProtocolPort> GrabProtocols;
	BarbaCourierRequestMode HttpRequestMode;
};
