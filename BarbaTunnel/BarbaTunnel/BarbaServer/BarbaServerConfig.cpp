#include "StdAfx.h"
#include "BarbaServerApp.h"
#include "BarbaServerConfig.h"

BarbaServerConfig::BarbaServerConfig()
{
}

bool BarbaServerConfig::LoadFile(LPCTSTR file)
{
	//AllowRequestBombard
	this->AllowRequestBombard = GetPrivateProfileInt(_T("General"), _T("AllowRequestBombard"), 1, file)!=0;
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
