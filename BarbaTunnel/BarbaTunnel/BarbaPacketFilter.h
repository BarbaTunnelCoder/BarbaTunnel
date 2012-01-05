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
	static void GetServerFilters(std::vector<STATIC_FILTER>* filters, BarbaServerConfig* config);
	static void GetBypassPacketFilter(std::vector<STATIC_FILTER>* filters);
	static void GetFilter(STATIC_FILTER* staticFilter, bool send, u_long ipStart, u_long ipEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd);
	static void AddFilter(std::vector<STATIC_FILTER>* filters, bool send, u_long ipStart, u_long ipEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd);
	static ULARGE_INTEGER GetAdapterHandle();
};

