#include "StdAfx.h"
#include "BarbaClientConfig.h"
#include "BarbaUtils.h"

BarbaClientConfig::BarbaClientConfig()
{
}

u_short BarbaClientConfig::GetNewTunnelPort()
{
	int newPortIndex = (rand()*rand() % GetTotalTunnelPortsCount());
	for (size_t i=0; i<this->TunnelPorts.size(); i++)
	{
		int count = TunnelPorts[i].EndPort - TunnelPorts[i].StartPort + 1;
		if (newPortIndex<count)
		{
			return (u_short)(TunnelPorts[i].StartPort + newPortIndex);
		}
		newPortIndex -= count;
	}

	return 0;
}

bool BarbaClientConfig::LoadFile(LPCTSTR file)
{
	if (!BarbaConfig::LoadFile(file))
		return false;

	//grabProtocols
	TCHAR grabProtocols[1000];
	GetPrivateProfileString(_T("General"), _T("GrabProtocols"), _T(""), grabProtocols, _countof(grabProtocols), file);
	BarbaUtils::GetProtocolAndPortArray(grabProtocols, &this->GrabProtocols);

	//FakePacketMinSize
	this->FakePacketMinSize = (u_short)GetPrivateProfileInt(_T("General"), _T("FakePacketMinSize"), 0, file);
	if (this->FakePacketMinSize>BARBA_HttpFakePacketMaxSize)
	{
		this->Log(_T("FakePacketMinSize could not be more than %d!"), BARBA_HttpFakePacketMaxSize);
		this->FakePacketMinSize = BARBA_HttpFakePacketMaxSize;
	}

	//KeepAliveInterval
	this->KeepAliveInterval = (size_t)GetPrivateProfileInt(_T("General"), _T("KeepAliveInterval"), BARBA_HttpKeepAliveInterval/1000, file) * 1000;
	if (this->KeepAliveInterval!=0 && this->KeepAliveInterval<BARBA_HttpKeepAliveIntervalMin)
	{
		this->Log(_T("KeepAliveInterval could not be less than %d!"), BARBA_HttpKeepAliveIntervalMin/1000);
		this->KeepAliveInterval = BARBA_HttpKeepAliveIntervalMin;
	}

	return true;
}

void BarbaClientConfig::LoadFolder(LPCTSTR folder, std::vector<BarbaClientConfig>* configs)
{
	std::vector<std::tstring> files;
	BarbaUtils::FindFiles(folder, _T("*.ini"), true, &files);
	for (size_t i=0; i<files.size(); i++)
	{
		BarbaClientConfig config;
		if (config.LoadFile(files[i].data()))
			configs->push_back(config);
	}
}
