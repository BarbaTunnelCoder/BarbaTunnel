#pragma once
#include "BarbaConfigItem.h"

class BarbaServerConfigItem : public BarbaConfigItem
{
public:
	BarbaServerConfigItem();
	virtual ~BarbaServerConfigItem(){}
};

class BarbaServerConfig
{
public:
	BarbaServerConfig();
	virtual ~BarbaServerConfig(){}
	DWORD AutoStartDelayMinutes;
	IpRange VirtualIpRange;
	BarbaKey Key;
	bool LoadFile(LPCTSTR file);

	BarbaServerConfigItem Items[BARBA_MAX_CONFIGITEMS];
	size_t ItemsCount;
};