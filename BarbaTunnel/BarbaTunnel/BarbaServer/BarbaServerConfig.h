#pragma once
#include "BarbaConfig.h"

class BarbaServerConfig : public BarbaConfig
{
public:
	BarbaServerConfig();
	virtual ~BarbaServerConfig(){}
	virtual bool LoadFile(LPCTSTR file);
	static void LoadFolder(LPCTSTR folder, std::vector<BarbaServerConfig>* configs);
};
