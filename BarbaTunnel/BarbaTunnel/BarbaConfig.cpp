#include "StdAfx.h"
#include "BarbaConfig.h"

BarbaConfig::BarbaConfig()
{
	GrabProtocolsCount = 0;
}

void BarbaConfig::LoadFile(LPCTSTR file)
{
	TCHAR serverAddress[255];
	GetPrivateProfileString(_T("General"), _T("ServerAddress"), _T(""), serverAddress, _countof(serverAddress), file);
	ServerIP = BarbaUtils::ConvertStringIp(serverAddress);
		
	//TunnelProtocol
	TCHAR tunnelProtocol[100];
	GetPrivateProfileString(_T("General"), _T("TunnelProtocol"), _T(""), tunnelProtocol, _countof(tunnelProtocol), file);
	BarbaUtils::GetProtocolAndPort(tunnelProtocol, &TunnelProtocol.Protocol, &TunnelProtocol.Port);
	if (TunnelProtocol.Protocol==0) TunnelProtocol.Protocol = IPPROTO_TCP;
	if (TunnelProtocol.Port==0) TunnelProtocol.Port = 80;
		
	//GrabProtocols
	TCHAR grabProtocols[1000];
	GetPrivateProfileString(_T("General"), _T("GrabProtocols"), _T(""), grabProtocols, _countof(grabProtocols), file);
	ParseGrabProtocols(grabProtocols);
}


void BarbaConfig::ParseGrabProtocols(LPCTSTR value)
{
	TCHAR buffer[1000];
	_tcscpy_s(buffer, value);

	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(buffer, _T(","), &currentPos);
		
	while (token!=NULL && GrabProtocolsCount<_countof(GrabProtocols))
	{
		TCHAR* end = NULL;
		ProtocolPort protocol;
		if (BarbaUtils::GetProtocolAndPort(token, &protocol.Protocol, &protocol.Port))
		{
			GrabProtocols[GrabProtocolsCount] = protocol;
			GrabProtocolsCount++;

		}
		token = _tcstok_s(NULL, _T(","), &currentPos);
	}
}


BarbaConfigManager::BarbaConfigManager()
{
	ConfigsCount = 0;
	ServerConfig = NULL;
}

BarbaConfigManager::~BarbaConfigManager()
{
	for (int i=0; i<ConfigsCount; i++)
		delete Configs[i];
}

BarbaConfig* BarbaConfigManager::GetServerConfig() 	
{ 
	return ServerConfig; 
}


void BarbaConfigManager::LoadFolder(LPCTSTR folder)
{
	TCHAR file[MAX_PATH];
	WIN32_FIND_DATA findData = {0};
		
	//findData.
	_tcscpy_s(file, _countof(file), folder);
	_tcsncat_s(file, _T("\\config\\*.cfg"), MAX_PATH);
	HANDLE findHandle = FindFirstFile(file, &findData);
	BOOL bfind = findHandle!=NULL;
	while (bfind)
	{
		if (ConfigsCount>=_countof(Configs))
			break;
		BarbaConfig* config = new BarbaConfig();
			
		TCHAR fullPath[MAX_PATH] = {0};
		_tcsncat_s(fullPath, folder, _countof(fullPath));
		_tcsncat_s(fullPath, _T("\\config\\"), _countof(fullPath));
		_tcsncat_s(fullPath, findData.cFileName, _countof(fullPath));

		config->LoadFile(fullPath);
		Configs[ConfigsCount++] = config;
		if (_tcsicmp(findData.cFileName, "server.cfg")==0)
			ServerConfig = config;
		bfind = FindNextFile(findHandle, &findData);
	}
	FindClose(findHandle);
}

BarbaConfig* BarbaConfigManager::FindByServerIP(DWORD ip)
{
	for (int i=0; i<ConfigsCount; i++)
		if (Configs[i]->ServerIP==ip)
			return Configs[i];
	return NULL;
}
