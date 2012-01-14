#pragma once
#include "BarbaConfigItem.h"

class BarbaServerConfig : public BarbaConfigItem
{
public:
	BarbaServerConfig();
	virtual ~BarbaServerConfig(){}
	virtual bool LoadFile(LPCTSTR file);
	static void LoadFolder(LPCTSTR folder, std::vector<BarbaServerConfig>* configs);
};
