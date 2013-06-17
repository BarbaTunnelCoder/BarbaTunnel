#include "StdAfx.h"
#include "BarbaClientConfig.h"
#include "BarbaUtils.h"

BarbaClientConfig::BarbaClientConfig()
{
	MaxTransferSize = BARBA_MaxTransferSize;
	KeepAliveInterval = BARBA_KeepAliveIntervalMin;
}

bool BarbaClientConfig::LoadFile(LPCTSTR file)
{
	if (!BarbaConfig::LoadFile(file))
		return false;

	//grabProtocols
	TCHAR grabProtocols[1000];
	GetPrivateProfileString(_T("General"), _T("GrabProtocols"), _T(""), grabProtocols, _countof(grabProtocols), file);
	BarbaUtils::GetProtocolAndPortArray(grabProtocols, &GrabProtocols);

	//Add RealPort to GrabProtocols
	if (Mode==BarbaModeTcpRedirect || Mode==BarbaModeUdpRedirect)
	{
		GrabProtocols.clear();
		ProtocolPort port;
		port.Protocol = GetTunnelProtocol();
		port.Port = RealPort;
		GrabProtocols.append(port);
	}
	
	//MinPacketSize
	MinPacketSize = (u_short)GetPrivateProfileInt(_T("General"), _T("MinPacketSize"), 0, file);
	if (MinPacketSize > BARBA_MinPacketSizeLimit)
	{
		Log(_T("MinPacketSize could not be more than %d!"), BARBA_MinPacketSizeLimit);
		MinPacketSize = BARBA_MinPacketSizeLimit;
	}

	//MaxPacketSize
	MaxPacketSize = (u_short)GetPrivateProfileInt(_T("General"), _T("MaxPacketSize"), 1500, file);
	KeepAlivePortsCount = (u_short)GetPrivateProfileInt(_T("General"), _T("KeepAlivePortsCount"), 100, file);

	//KeepAliveInterval
	KeepAliveInterval = (size_t)GetPrivateProfileInt(_T("General"), _T("KeepAliveInterval"), BARBA_KeepAliveIntervalDefault/1000, file) * 1000;
	if (KeepAliveInterval!=0 && KeepAliveInterval<BARBA_KeepAliveIntervalMin)
	{
		Log(_T("KeepAliveInterval could not be less than %d!"), BARBA_KeepAliveIntervalMin/1000);
		KeepAliveInterval = BARBA_KeepAliveIntervalMin;
	}

	//Http-Bombard
	TCHAR httpRequestMode[1000] = {0};
	GetPrivateProfileString(_T("General"), _T("HttpRequestMode"), _T(""), httpRequestMode, _countof(httpRequestMode), file);
	std::tstring strRequest = httpRequestMode;
	StringUtils::Trim(strRequest);
	if (strRequest.empty()) strRequest = (Mode == BarbaModeHttpTunnel) ? _T("Normal") : _T("None");
	HttpRequestMode.Parse(strRequest);

	//validate tunnel mode and httpRequest
	if (Mode==BarbaModeHttpTunnel && HttpRequestMode.Mode == BarbaCourierRequestMode::ModeNone)
	{
		Log(_T("Invalid HttpRequestMode for HTTP-Tunnel, HttpRequestMode treat as Normal!"));
		HttpRequestMode.Mode = BarbaCourierRequestMode::ModeNormal;
	}
	if (Mode==BarbaModeTcpTunnel && HttpRequestMode.Mode != BarbaCourierRequestMode::ModeNone)
	{
		Mode = BarbaModeHttpTunnel;
	}

	//MaxTransferSize
	MaxTransferSize = (u_short)GetPrivateProfileInt(_T("General"), _T("MaxTransferSize"), MaxTransferSize/1000, file) * 1000;
	if (MaxTransferSize==0) MaxTransferSize = BARBA_MaxTransferSize;

	//FakeFileTypes
	TCHAR fakeFileTypes[1000] = {0};
	GetPrivateProfileString(_T("General"), _T("FakeFileTypes"), _T(""), fakeFileTypes, _countof(fakeFileTypes), file);
	StringUtils::Tokenize(fakeFileTypes, _T(","), &FakeFileTypes);

	return true;
}

void BarbaClientConfig::LoadFolder(LPCTSTR folder, BarbaArray<BarbaClientConfig>* configs)
{
	std::vector<std::tstring> files;
	BarbaUtils::FindFiles(folder, _T("*.ini"), true, &files);
	for (size_t i=0; i<files.size(); i++)
	{
		BarbaClientConfig config;
		if (config.LoadFile(files[i].data()))
			configs->append(config);
	}
}
