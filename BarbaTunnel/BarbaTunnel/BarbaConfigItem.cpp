#include "stdafx.h"
#include "BarbaConfigItem.h"
#include "BarbaUtils.h"
#include "BarbaCrypt.h"

BarbaConfigItem::BarbaConfigItem()
{
	Mode = BarbaModeNone;
	Name[0] = 0;
	Enabled = true;
	RealPort = 0;
	_TotalTunnelPortsCount = 0;
	MaxUserConnections = BARBA_HttpMaxUserConnection;
	FakeFileMaxSize = BARBA_HttpMaxFakeFileSize;
}


size_t BarbaConfigItem::GetTotalTunnelPortsCount()
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

bool BarbaConfigItem::Load(LPCTSTR sectionName, LPCTSTR file)
{
	this->FileName = BarbaUtils::GetFileNameFromUrl(file);
	this->SectionName = sectionName;

	//name
	GetPrivateProfileString(sectionName, _T("Name"), _T(""), this->Name, _countof(this->Name), file);

	//fail if not enabled
	this->Enabled = GetPrivateProfileInt(sectionName, "Enabled", 1, file)!=0;
	if (!this->Enabled)
		return false;

	//mode
	TCHAR modeString[100];
	int res = GetPrivateProfileString(sectionName, _T("Mode"), _T(""), modeString, _countof(modeString), file);
	if (res==0)
		return false; //could not find item

	this->Mode = BarbaMode_FromString(modeString);
	if (this->Mode==BarbaModeTcpTunnel || this->Mode==BarbaModeNone)
	{
		Log(_T("Error: %s mode not supported!"), modeString);
		return false;
	}

	//Key
	TCHAR hexKey[2000];
	GetPrivateProfileString(sectionName, _T("Key"), _T(""), hexKey, _countof(hexKey), file);
	BarbaUtils::ConvertHexStringToBuffer(hexKey, &this->Key);

	//TunnelPorts
	TCHAR tunnelPorts[2000];
	GetPrivateProfileString(sectionName, _T("TunnelPorts"), _T(""), tunnelPorts, _countof(tunnelPorts), file);
	BarbaUtils::ParsePortRanges(tunnelPorts, &this->TunnelPorts);
	if (this->GetTotalTunnelPortsCount()==0)
	{
		Log(_T("Error: Item does not specify any tunnel port!"));
		return false;
	}

	//MaxUserConnections
	this->MaxUserConnections = (u_short)GetPrivateProfileInt(sectionName, _T("MaxUserConnections"), this->MaxUserConnections, file);
	CheckMaxUserConnections();

	//FakeFileMaxSize
	this->FakeFileMaxSize = (u_short)GetPrivateProfileInt(sectionName, _T("FakeFileMaxSize"), this->FakeFileMaxSize, file) * 1000;
	if (this->FakeFileMaxSize==0) this->FakeFileMaxSize = BARBA_HttpMaxFakeFileSize * 1000;

	TCHAR keyName[BARBA_MaxKeyName];
	keyName[0] = 0;
	GetPrivateProfileString(sectionName, _T("RequestDataKeyName"), _T(""), keyName, _countof(keyName), file);
	this->RequestDataKeyName = keyName;
	if (this->RequestDataKeyName.empty())
		this->RequestDataKeyName = CreateRequestDataKeyName(&this->Key);

	//FakeFileTypes
	TCHAR fakeFileTypes[1000] = {0};
	GetPrivateProfileString(sectionName, _T("FakeFileTypes"), _T(""), fakeFileTypes, _countof(fakeFileTypes), file);
	StringUtils::Tokenize(fakeFileTypes, _T(","), &this->FakeFileTypes);

	//RealPort
	this->RealPort = (u_short)GetPrivateProfileInt(sectionName, _T("RealPort"), 0, file);
	return true;
}

void BarbaConfigItem::CheckMaxUserConnections()
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

void BarbaConfigItem::Log(LPCTSTR format, ...)
{
	va_list argp;
	va_start(argp, format);
	TCHAR msg[1000];
	_vstprintf_s(msg, format, argp);
	va_end(argp);

	TCHAR msg2[1000];
	_stprintf_s(msg2, _T("ConfigLoader: %s [%s]: %s"), this->FileName.data(), this->SectionName.data(), msg);
	BarbaLog2(msg2);
}


std::tstring BarbaConfigItem::CreateRequestDataKeyName(std::vector<BYTE>* key)
{
	std::string keyName = "BData";
	//add some 'A' to change they KeyName size depending to key len
	for (int i=0; i<(int)key->size()/2; i++)
		keyName.push_back( 'A' );

	std::vector<BYTE> keyBuffer(keyName.size());
	memcpy_s(&keyBuffer.front(), keyBuffer.size(), keyName.data(), keyName.size());

	BarbaCrypt::Crypt(&keyBuffer.front(), keyBuffer.size(), key->data(), key->size(), true);
	std::tstring ret = Base64::encode(&keyBuffer);
	StringUtils::ReplaceAll(ret, "=", "");
	StringUtils::ReplaceAll(ret, "/", "");
	return ret;

}