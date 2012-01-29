#include "stdafx.h"
#include "BarbaConfig.h"
#include "BarbaUtils.h"
#include "BarbaCrypt.h"

BarbaConfig::BarbaConfig()
{
	this->ServerIp = 0;
	this->Mode = BarbaModeNone;
	this->Enabled = true;
	this->RealPort = 0;
	this->_TotalTunnelPortsCount = 0;
	this->MaxUserConnections = BARBA_HttpMaxUserConnection;
	this->FakeFileMaxSize = BARBA_HttpFakeFileMaxSize;
}


size_t BarbaConfig::GetTotalTunnelPortsCount()
{
	if (_TotalTunnelPortsCount==0)
	{
		for (size_t i=0; i<this->TunnelPorts.size(); i++)
		{
			int count = TunnelPorts[i].EndPort - TunnelPorts[i].StartPort + 1;
			if (count>0)
				_TotalTunnelPortsCount += count;
		}
	}
	return _TotalTunnelPortsCount;
}

std::tstring BarbaConfig::GetNameFromFileName(LPCTSTR fileName)
{
	std::tstring ret = BarbaUtils::GetFileTitleFromUrl(fileName);
	std::tstring parentFolder = BarbaUtils::GetFileFolderFromUrl(fileName);
	std::tstring parentFolderName = BarbaUtils::GetFileNameFromUrl(parentFolder.data());
	while ( !parentFolderName.empty() && _tcsicmp(parentFolderName.data(), BARBA_ConfigFolderName)!=0 )
	{
		TCHAR name[MAX_PATH];
		_stprintf_s(name, _T("%s\\%s"), parentFolderName.data(), ret.data());
		ret = name;
		parentFolder = BarbaUtils::GetFileFolderFromUrl(parentFolder.data());
		parentFolderName = BarbaUtils::GetFileNameFromUrl(parentFolder.data());
	}
	return ret;
}



bool BarbaConfig::LoadFile(LPCTSTR file)
{
	this->FileName = BarbaUtils::GetFileNameFromUrl(file);
	this->Name = GetNameFromFileName(file);

	//ServerIp
	TCHAR serverAddress[1000];
	GetPrivateProfileString(_T("General"), _T("ServerAddress"), _T("*"), serverAddress, _countof(serverAddress), file);
	this->ServerAddress = serverAddress;
	if (this->ServerAddress.empty() || this->ServerAddress.compare(_T("*"))==0)
		this->ServerAddress = _T("*");
	hostent *he = gethostbyname(serverAddress);
	this->ServerIp = he!=NULL ?  ((in_addr *)he->h_addr)->S_un.S_addr : 0;

	//fail if not enabled
	this->Enabled = GetPrivateProfileInt(_T("General"), _T("Enabled"), 1, file)!=0;
	if (!this->Enabled)
		return false;

	//mode
	TCHAR modeString[100];
	int res = GetPrivateProfileString(_T("General"), _T("Mode"), _T(""), modeString, _countof(modeString), file);
	if (res==0)
		return false; //could not find item

	this->Mode = BarbaMode_FromString(modeString);
	if (this->Mode==BarbaModeTcpTunnel)
	{
		Log(_T("TCP-Tunnel suppressed by HTTP-Tunnel. HTTP-Tunnel is used as an advanced TCP-Tunnel."));
		this->Mode = BarbaModeHttpTunnel;
	}

	//Key
	TCHAR hexKey[2000];
	GetPrivateProfileString(_T("General"), _T("Key"), _T(""), hexKey, _countof(hexKey), file);
	BarbaUtils::ConvertHexStringToBuffer(hexKey, &this->Key);

	//TunnelPorts
	TCHAR tunnelPorts[2000];
	GetPrivateProfileString(_T("General"), _T("TunnelPorts"), _T(""), tunnelPorts, _countof(tunnelPorts), file);
	BarbaUtils::ParsePortRanges(tunnelPorts, &this->TunnelPorts);
	if (this->GetTotalTunnelPortsCount()==0)
	{
		Log(_T("Error: Item does not specify any tunnel port!"));
		return false;
	}

	//MaxUserConnections
	this->MaxUserConnections = (u_short)GetPrivateProfileInt(_T("General"), _T("MaxUserConnections"), this->MaxUserConnections, file);
	CheckMaxUserConnections();

	//FakeFileMaxSize
	this->FakeFileMaxSize = (u_short)GetPrivateProfileInt(_T("General"), _T("FakeFileMaxSize"), this->FakeFileMaxSize/1000, file) * 1000;
	if (this->FakeFileMaxSize==0) this->FakeFileMaxSize = BARBA_HttpFakeFileMaxSize;

	TCHAR keyName[BARBA_MaxKeyName];
	keyName[0] = 0;
	GetPrivateProfileString(_T("General"), _T("RequestDataKeyName"), _T(""), keyName, _countof(keyName), file);
	this->RequestDataKeyName = keyName;
	if (this->RequestDataKeyName.empty())
		this->RequestDataKeyName = CreateRequestDataKeyName(&this->Key);

	//FakeFileTypes
	TCHAR fakeFileTypes[1000] = {0};
	GetPrivateProfileString(_T("General"), _T("FakeFileTypes"), _T(""), fakeFileTypes, _countof(fakeFileTypes), file);
	StringUtils::Tokenize(fakeFileTypes, _T(","), &this->FakeFileTypes);

	//RealPort
	this->RealPort = (u_short)GetPrivateProfileInt(_T("General"), _T("RealPort"), 0, file);
	return true;
}

void BarbaConfig::CheckMaxUserConnections()
{
	if (this->MaxUserConnections==0)
	{
		Log(_T("Warning: Item specify %d MaxUserConnections! It should be 2 or more."), this->MaxUserConnections);
		this->MaxUserConnections = 2;
	}
	if (this->MaxUserConnections==1)
	{
		Log(_T("Warning: Item specify %d MaxUserConnections! It strongly recommended to be 2 or more."), this->MaxUserConnections);
	}
	if (this->MaxUserConnections>20)
	{
		Log(_T("Warning: Item specify %d MaxUserConnections! It could not be more than %d."), this->MaxUserConnections, BARBA_HttpMaxUserConnections);
		this->MaxUserConnections = BARBA_HttpMaxUserConnections;
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
	_stprintf_s(msg2, _T("ConfigLoader: %s: %s"), this->FileName.data(), msg);
	BarbaLog2(msg2);
}


std::tstring BarbaConfig::CreateRequestDataKeyName(std::vector<BYTE>* key)
{
	std::string keyName = "BData";
	//add some 'A' to change they KeyName size depending to key len
	for (size_t i=0; i<key->size()/2; i++)
		keyName.push_back( 'A' );

	std::vector<BYTE> keyBuffer(keyName.size());
	memcpy_s(&keyBuffer.front(), keyBuffer.size(), keyName.data(), keyName.size());

	BarbaCrypt::Crypt(&keyBuffer, key, true);
	std::tstring ret = Base64::encode(&keyBuffer);
	StringUtils::ReplaceAll(ret, "=", "");
	StringUtils::ReplaceAll(ret, "/", "");
	return ret;

}