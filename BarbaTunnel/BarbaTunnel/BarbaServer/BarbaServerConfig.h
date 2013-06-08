#pragma once
#include "BarbaConfig.h"

class BarbaServerConfig : public BarbaConfig
{
public:
	BarbaServerConfig();
	virtual ~BarbaServerConfig(){}
	bool LoadFile(LPCTSTR file) override;
	static void LoadFolder(LPCTSTR folder, BarbaArray<BarbaServerConfig>* configs);
	
	bool AllowHttpRequestNone;
	bool AllowHttpRequestNormal;
	bool AllowHttpRequestBombard;
};
