#include "StdAfx.h"
#include "BarbaClientConfig.h"
#include "BarbaUtils.h"

BarbaClientConfig::BarbaClientConfig()
{
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
	this->MinPacketSize = (u_short)GetPrivateProfileInt(_T("General"), _T("MinPacketSize"), 0, file);
	if (this->MinPacketSize > BARBA_HttpMaxPacketSize)
	{
		this->Log(_T("MinPacketSize could not be more than %d!"), BARBA_HttpMaxPacketSize);
		this->MinPacketSize = BARBA_HttpMaxPacketSize;
	}

	//KeepAliveInterval
	this->KeepAliveInterval = (size_t)GetPrivateProfileInt(_T("General"), _T("KeepAliveInterval"), BARBA_HttpKeepAliveInterval/1000, file) * 1000;
	if (this->KeepAliveInterval!=0 && this->KeepAliveInterval<BARBA_HttpKeepAliveIntervalMin)
	{
		this->Log(_T("KeepAliveInterval could not be less than %d!"), BARBA_HttpKeepAliveIntervalMin/1000);
		this->KeepAliveInterval = BARBA_HttpKeepAliveIntervalMin;
	}

	//Http-Bombard
	TCHAR httpRequestMode[1000] = {0};
	GetPrivateProfileString(_T("General"), _T("HttpRequestMode"), _T(""), httpRequestMode, _countof(httpRequestMode), file);
	std::tstring strRequest = httpRequestMode;
	StringUtils::Trim(strRequest);
	if (strRequest.empty()) strRequest = (Mode == BarbaModeHttpTunnel) ? _T("Normal") : _T("None");
	HttpRequestMode.Parse(strRequest);

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
