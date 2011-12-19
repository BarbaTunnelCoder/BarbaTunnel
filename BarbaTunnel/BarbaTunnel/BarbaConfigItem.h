#pragma once
#include "General.h"

class BarbaConfigItem
{
public:
	BarbaConfigItem();
	virtual ~BarbaConfigItem() {}
	u_short RealPort; //valid when mode is UDP-Redirect or TCP-Redirect mode
	size_t GetTotalTunnelPortsCount();
	u_char GetTunnelProtocol() {return BarbaMode_GetProtocol(this->Mode);}
	bool IsTunnelMode() { return Mode==BarbaModeTcpTunnel || Mode==BarbaModeUdpTunnel; }
	bool IsRedirectMode() { return Mode==BarbaModeTcpRedirect || Mode==BarbaModeTcpRedirect; }
	BarbaModeEnum Mode;
	PortRange TunnelPorts[BARBA_MAX_PORTITEM];
	size_t TunnelPortsCount;
	TCHAR Name[BARBA_MAX_CONFIGNAME];
	u_short MaxUserConnections; //use by HTTP-Tunnel
	bool Enabled;
	bool Load(LPCTSTR sectionName, LPCTSTR file);

private:
	void CheckMaxUserConnections();
	int _TotalTunnelPortsCount;
};
