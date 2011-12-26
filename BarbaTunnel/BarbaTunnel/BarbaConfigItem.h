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
	std::vector<PortRange> TunnelPorts;
	TCHAR Name[BARBA_MaxConfigName];
	u_short MaxUserConnections; //use by HTTP-Tunnel
	bool Enabled;
	bool Load(LPCTSTR sectionName, LPCTSTR file);
	std::vector<std::tstring> FakeFileTypes;
	std::vector<BYTE> Key;
	u_int FakeFileMaxSize;
	std::tstring FakeFileHeaderSizeKeyName;
	std::tstring SessionKeyName;

private:
	void Log(LPCTSTR format, ...);
	void CheckMaxUserConnections();
	int _TotalTunnelPortsCount;
	std::tstring FileName; //used for report
	std::tstring SectionName; //used for report
};
