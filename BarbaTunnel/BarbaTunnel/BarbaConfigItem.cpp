#include "stdafx.h"
#include "BarbaConfigItem.h"
#include "BarbaUtils.h"

BarbaConfigItem::BarbaConfigItem()
{
	Mode = BarbaModeNone;
	TunnelPortsCount = 0;
	memset(this->TunnelPorts, 0, sizeof(this->TunnelPorts));
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
		for (size_t i=0; i<TunnelPortsCount; i++)
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
		BarbaLog(_T("Error: %s mode not supported!"), modeString);
		return false;
	}

	//TunnelPorts
	TCHAR tunnelPorts[BARBA_MAX_PORTITEM*10];
	GetPrivateProfileString(sectionName, _T("TunnelPorts"), _T(""), tunnelPorts, _countof(tunnelPorts), file);
	this->TunnelPortsCount = BarbaUtils::ParsePortRanges(tunnelPorts, this->TunnelPorts, _countof(this->TunnelPorts));
	if (this->GetTotalTunnelPortsCount()==0)
	{
		BarbaLog(_T("Error: %s item does not specify any tunnel port!"), this->Name);
		return false;
	}

	//MaxUserConnections
	this->MaxUserConnections = (u_short)GetPrivateProfileInt(sectionName, _T("MaxUserConnections"), this->MaxUserConnections, file);
	CheckMaxUserConnections();

	//MaxUserConnections
	this->FakeFileMaxSize = (u_short)GetPrivateProfileInt(sectionName, _T("FakeFileMaxSize"), this->FakeFileMaxSize, file) * 1000;
	if (this->FakeFileMaxSize==0) this->FakeFileMaxSize = BARBA_HttpMaxFakeFileSize * 1000;

	TCHAR keyName[BARBA_MaxKeyName];
	//FakeFileHeaderSizeKeyName
	keyName[0] = 0;
	GetPrivateProfileString(sectionName, _T("FakeFileHeaderSizeKeyName"), _T("HeaderLen"), keyName, _countof(keyName), file);
	this->FakeFileHeaderSizeKeyName = keyName;

	//FakeFileHeaderSizeKeyName
	keyName[0] = 0;
	GetPrivateProfileString(sectionName, _T("SessionKeyName"), _T("session"), keyName, _countof(keyName), file);
	this->SessionKeyName = keyName;

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
		BarbaLog(_T("Warning: %s item specify %d MaxUserConnections! It should be 2 or more."), this->Name, this->MaxUserConnections);
		this->MaxUserConnections = 2;
	}
	if (this->MaxUserConnections==1)
	{
		BarbaLog(_T("Warning: %s item specify %d MaxUserConnections! It strongly recommended to be 2 or more."), this->Name, this->MaxUserConnections);
	}
	if (this->MaxUserConnections>20)
	{
		BarbaLog(_T("Warning: %s item specify %d MaxUserConnections! It could not be more than %d."), this->Name, this->MaxUserConnections, BARBA_HttpMaxUserConnections);
		this->MaxUserConnections = BARBA_HttpMaxUserConnections;
	}
}
