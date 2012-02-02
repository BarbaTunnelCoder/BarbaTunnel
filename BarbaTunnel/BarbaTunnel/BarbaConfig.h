#pragma once
#include "General.h"

class BarbaConfig
{
public:
	BarbaConfig();
	virtual ~BarbaConfig() {}
	virtual bool LoadFile(LPCTSTR file);
	void Log(LPCTSTR format, ...);
	bool Enabled;
	std::tstring ServerAddress;
	DWORD ServerIp;
	std::tstring Name;
	BarbaModeEnum Mode;
	BarbaBuffer Key; 
	size_t GetTotalTunnelPortsCount();
	u_char GetTunnelProtocol() {return BarbaMode_GetProtocol(this->Mode);}
	bool IsTunnelMode() { return Mode==BarbaModeTcpTunnel || Mode==BarbaModeUdpTunnel; }
	bool IsRedirectMode() { return Mode==BarbaModeTcpRedirect || Mode==BarbaModeTcpRedirect; }
	std::vector<PortRange> TunnelPorts;
	u_short RealPort; //valid when mode is UDP-Redirect or TCP-Redirect mode
	u_short MaxUserConnections; //use by HTTP-Tunnel
	std::vector<std::tstring> FakeFileTypes; //use by HTTP-Tunnel
	u_int FakeFileMaxSize; //use by HTTP-Tunnel
	std::tstring RequestDataKeyName; //use by HTTP-Tunnel

private:
	static std::tstring GetNameFromFileName(LPCTSTR fileName);
	static std::tstring CreateRequestDataKeyName(BarbaBuffer* key);
	void CheckMaxUserConnections();
	int _TotalTunnelPortsCount;
	std::tstring FileName; //used for report
};
