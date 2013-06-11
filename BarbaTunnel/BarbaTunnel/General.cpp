#include "stdafx.h"
#include "General.h"
#include "BarbaCrypt.h"

// {E0B6D3F2-5B74-46E7-BC2C-5C349055DD06}
GUID BarbaSign = { 0xe0b6d3f2, 0x5b74, 0x46e7, { 0xbc, 0x2c, 0x5c, 0x34, 0x90, 0x55, 0xdd, 0x6 } };
GUID* GetBarbaSign()
{
	return &BarbaSign;
}

BarbaModeEnum BarbaMode_FromString(LPCTSTR mode)
{
	if (_tcsicmp(mode, _T("UDP-Tunnel"))==0) return BarbaModeUdpTunnel;
	if (_tcsicmp(mode, _T("UDP-SimpleTunnel"))==0) return BarbaModeUdpSimpleTunnel;
	else if (_tcsicmp(mode, _T("TCP-Tunnel"))==0) return BarbaModeTcpTunnel;
	else if (_tcsicmp(mode, _T("HTTP-Tunnel"))==0) return BarbaModeHttpTunnel;
	else if (_tcsicmp(mode, _T("UDP-Redirect"))==0) return BarbaModeUdpRedirect;
	else if (_tcsicmp(mode, _T("TCP-Redirect"))==0) return BarbaModeTcpRedirect;
	else return BarbaModeNone;
}

 LPCTSTR BarbaMode_ToString(BarbaModeEnum mode)
{
	switch(mode){
	case BarbaModeUdpTunnel: return _T("UDP-Tunnel");
	case BarbaModeUdpSimpleTunnel: return _T("UDP-SimpleTunnel");
	case BarbaModeHttpTunnel: return _T("HTTP-Tunnel");
	case BarbaModeTcpTunnel: return _T("TCP-Tunnel");
	case BarbaModeUdpRedirect: return _T("UDP-Redirect");
	case BarbaModeTcpRedirect: return _T("TCP-Redirect");
	default: return _T("None");
	}
}

u_char BarbaMode_GetProtocol(BarbaModeEnum mode)
{
	switch (mode){
	case BarbaModeUdpTunnel: return IPPROTO_UDP;
	case BarbaModeUdpSimpleTunnel: return IPPROTO_UDP;
	case BarbaModeUdpRedirect: return IPPROTO_UDP;
	case BarbaModeHttpTunnel: return IPPROTO_TCP;
	case BarbaModeTcpTunnel: return IPPROTO_TCP;
	case BarbaModeTcpRedirect: return IPPROTO_TCP;
	default: return IPPROTO_IP;
	}
}
