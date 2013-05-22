#pragma once
#include "General.h"
#include "BarbaPortRange.h"

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
	u_char GetTunnelProtocol() {return BarbaMode_GetProtocol(this->Mode);}
	BarbaPortRange TunnelPorts;
	u_short RealPort; //valid when mode is UDP-Redirect or TCP-Redirect mode
	u_short MaxUserConnections; //use by HTTP-Tunnel

private:
	static std::tstring GetNameFromFileName(LPCTSTR fileName);
	void CheckMaxUserConnections();
	std::tstring FileName; //used for report
};
