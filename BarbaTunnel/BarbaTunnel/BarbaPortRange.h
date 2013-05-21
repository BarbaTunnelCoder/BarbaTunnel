#pragma once
#include "General.h"

class BarbaPortRange
{
public:
	//PortRange
	struct PortRangeItem
	{
		u_short StartPort;
		u_short EndPort;
	};

public:
	BarbaPortRange(void);
	~BarbaPortRange(void);
	size_t GetPortsCount();
	u_short GetRandomPort();
	bool IsPortInRange(u_short port);
	void GetAllPorts(BarbaArray<u_short>* ports);
	//@param value: StartPort-EndPort,StartPort-EndPort,StartPort-EndPort
	void Parse(LPCTSTR value);
	static bool ParsePortRangeItem(LPCTSTR value, u_short* startPort, u_short* endPort);

	std::vector<PortRangeItem> Items;
};

