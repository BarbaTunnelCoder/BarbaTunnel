#include "stdafx.h"
#include "BarbaConfig.h"
#include "BarbaUtils.h"
#include "BarbaCrypt.h"
#include "BarbaApp.h"

BarbaConfig::BarbaConfig()
{
	ServerIp = INADDR_ANY;
	Mode = BarbaModeNone;
	Enabled = true;
	RealPort = 0;
	MaxUserConnections = BARBA_MaxUserConnectionsDefault;
}


std::tstring BarbaConfig::GetNameFromFileName(LPCTSTR fileName, bool containsFolder)
{
	std::tstring ret = BarbaUtils::GetFileTitleFromUrl(fileName);
	std::tstring parentFolder = BarbaUtils::GetFileFolderFromUrl(fileName);
	std::tstring parentFolderName = BarbaUtils::GetFileNameFromUrl(parentFolder.data());
	while ( !parentFolderName.empty() && _tcsicmp(parentFolderName.data(), BARBA_ConfigFolderName)!=0 )
	{
		TCHAR name[MAX_PATH];
		_stprintf_s(name, _T("%s"), ret.data());
		if (containsFolder)
			_stprintf_s(name, _T("%s\\%s"), parentFolderName.data(), ret.data());
		ret = name;
		parentFolder = BarbaUtils::GetFileFolderFromUrl(parentFolder.data());
		parentFolderName = BarbaUtils::GetFileNameFromUrl(parentFolder.data());
	}
	return ret;
}

std::tstring BarbaConfig::GetName(bool anonymously)
{
	return anonymously ? NameAnonymous : Name;
}


bool BarbaConfig::LoadFile(LPCTSTR file)
{
	FileName = BarbaUtils::GetFileNameFromUrl(file);
	Name = GetNameFromFileName(file, true);
	NameAnonymous = GetNameFromFileName(file, false);

	//fail if not enabled
	Enabled = GetPrivateProfileInt(_T("General"), _T("Enabled"), 1, file)!=0;
	if (!Enabled)
		return false;

	//ServerIp
	TCHAR serverAddress[1000];
	GetPrivateProfileString(_T("General"), _T("ServerAddress"), _T("*"), serverAddress, _countof(serverAddress), file);
	ServerAddress = serverAddress;
	StringUtils::Trim(ServerAddress);
	if (ServerAddress.empty() || ServerAddress.compare(_T("*"))==0)
	{
		ServerAddress = _T("*");
	}
	else
	{
		hostent *he = gethostbyname(serverAddress);
		if (he==NULL)
		{
			Log(_T("Error: Could not resolve server address! %s"), serverAddress);
			return false;
		}
		ServerIp = ((in_addr *)he->h_addr)->S_un.S_addr;
	}

	//mode
	TCHAR modeString[100];
	int res = GetPrivateProfileString(_T("General"), _T("Mode"), _T(""), modeString, _countof(modeString), file);
	if (res==0)
		return false; //could not find item
	Mode = BarbaMode_FromString(modeString);
	if (Mode==BarbaModeNone)
	{
		Log(_T("Unknown Tunnel Mode: %s"), modeString);
		return false;
	}
	
	//Key
	TCHAR hexKey[2000];
	GetPrivateProfileString(_T("General"), _T("Key"), _T(""), hexKey, _countof(hexKey), file);
	BarbaUtils::ConvertHexStringToBuffer(hexKey, &Key);

	//TunnelPorts
	TCHAR tunnelPorts[2000];
	GetPrivateProfileString(_T("General"), _T("TunnelPorts"), _T(""), tunnelPorts, _countof(tunnelPorts), file);
	TunnelPorts.Parse(tunnelPorts);
	if (TunnelPorts.GetPortsCount()==0)
	{
		Log(_T("Error: Item does not specify any tunnel port!"));
		return false;
	}

	//MaxUserConnections
	MaxUserConnections = (u_short)GetPrivateProfileInt(_T("General"), _T("MaxUserConnections"), MaxUserConnections, file);
	CheckMaxUserConnections();

	//RealPort
	RealPort = (u_short)GetPrivateProfileInt(_T("General"), _T("RealPort"), 0, file);
	return true;
}

void BarbaConfig::CheckMaxUserConnections()
{
	if (MaxUserConnections==0)
	{
		Log(_T("Warning: Item specify %d MaxUserConnections! It should be 2 or more."), MaxUserConnections);
		MaxUserConnections = 2;
	}
	if (MaxUserConnections==1)
	{
		Log(_T("Warning: Item specify %d MaxUserConnections! It strongly recommended to be 2 or more."), MaxUserConnections);
	}
	if (MaxUserConnections>BARBA_MaxUserConnections)
	{
		Log(_T("Warning: Item specify %d MaxUserConnections! It could not be more than %d."), MaxUserConnections, BARBA_MaxUserConnections);
		MaxUserConnections = BARBA_MaxUserConnections;
	}
}

void BarbaConfig::Log(LPCTSTR format, ...)
{
	va_list argp;
	va_start(argp, format);
	TCHAR msg[1000];
	_vstprintf_s(msg, format, argp);
	va_end(argp);

	TCHAR msg2[1000];
	_stprintf_s(msg2, _T("ConfigLoader: %s: %s"), FileName.data(), msg);
	BarbaLog2(msg2);
}
