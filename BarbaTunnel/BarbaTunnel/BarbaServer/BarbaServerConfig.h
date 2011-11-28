#pragma once
#include "BarbaConfigItem.h"

class BarbaServerConfigItem : public BarbaConfigItem
{
public:
	BarbaServerConfigItem();
};

class BarbaServerConfig
{
public:
	BarbaServerConfig();
	DWORD AutoStartDelayMinutes;
	IpRange VirtualIpRange;
	BYTE Key[BARBA_MAX_KEYLEN];
	size_t KeyCount;
	bool LoadFile(LPCTSTR file);

	BarbaServerConfigItem Items[BARBA_MAX_CONFIGITEMS];
	size_t ItemsCount;
};