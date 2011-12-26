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
	bool LoadFile(LPCTSTR file);
	std::vector<BarbaServerConfigItem> Items;
};