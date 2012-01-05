#pragma once
#include "General.h"
#include "BarbaClient\BarbaClientConfig.h"

class BarbaPacketFilter
{
public:
	static bool ApplyPacketFilter();

private: 
	static void ApplyClientPacketFilter();
	static void ApplyServerPacketFilter();
	static bool ApplyFilters(std::vector<STATIC_FILTER>* filters);
	static void GetClientFilters(std::vector<STATIC_FILTER>* filters, BarbaClientConfig* config);
	static void GetClientGrabFilters(std::vector<STATIC_FILTER>* filters, u_long remoteIp, std::vector<ProtocolPort>* protocolPorts);
	static void GetClientTunnelFilters(std::vector<STATIC_FILTER>* filters, u_long remoteIp, u_char protocol, std::vector<PortRange>* portRanges);
	static void GetBypassPacketFilter(std::vector<STATIC_FILTER>* filters);
	static void GetFilter(STATIC_FILTER* staticFilter, bool send, u_long startIp, u_long endIp, u_char protocol, PortRange* srcPortRange, PortRange* desPortRange);
	static ULARGE_INTEGER GetAdapterHandle();
};

