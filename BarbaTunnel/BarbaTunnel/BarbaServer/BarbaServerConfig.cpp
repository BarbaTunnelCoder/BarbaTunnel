#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConfig.h"

BarbaServerConfig::BarbaServerConfig()
{
}

bool BarbaServerConfig::LoadFile(LPCTSTR file)
{
	return 	BarbaConfig::LoadFile(file);
}

void BarbaServerConfig::LoadFolder(LPCTSTR folder, std::vector<BarbaServerConfig>* configs)
{
	std::vector<std::tstring> files;
	BarbaUtils::FindFiles(folder, _T("*.ini"), true, &files);
	for (size_t i=0; i<files.size(); i++)
	{
		BarbaServerConfig config;
		if (config.LoadFile(files[i].data()))
			configs->push_back(config);
	}
}
