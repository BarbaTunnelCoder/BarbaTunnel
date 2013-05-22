#include "stdafx.h"
#include "BarbaCourierRequestMode.h"
#include "StringUtils.h"
#include "BarbaUtils.h"

BarbaCourierRequestMode::BarbaCourierRequestMode(void)
{
	Reset();
}

void BarbaCourierRequestMode::Reset()
{
	Mode = ModeNone;
	BombardGet = false;
	BombardGetPayload = false;
	BombardPost = false;
	BombardPostReply = false;
	BombardPostReplyPayload = false;
}

bool BarbaCourierRequestMode::IsFullBombard()
{
	return BombardGet && BombardGetPayload && BombardPost && BombardPostReply && BombardPostReplyPayload;
}


std::tstring BarbaCourierRequestMode::ToString()
{
	std::tstring ret;
	if (Mode==ModeNone) return _T("None");
	if (Mode==ModeNormal) return _T("Normal");
	if (IsFullBombard()) return _T("Bombard");

	if (BombardGet) ret += _T(" /get");
	if (BombardGetPayload) ret += _T(" /getPayload");
	if (BombardPost) ret += _T(" /post");
	if (BombardPostReply) ret += _T(" /postReply");
	if (BombardPostReplyPayload) ret += _T(" /postReplyPayload");
	StringUtils::Trim(ret);
	return ret;
}

void BarbaCourierRequestMode::Parse(std::tstring valueParam)
{
	Reset();
	StringUtils::Trim(valueParam);
	StringUtils::MakeLower(valueParam);
	valueParam.append(_T(" "));
	LPCTSTR value = valueParam.data();

	if (_tcscmp(value, _T("0"))==0 || _tcscmp(value, _T("none "))==0 || _tcscmp(value, _T(""))==0) Mode=ModeNone;
	else if (_tcscmp(value, _T("1"))==0 || _tcscmp(value, _T("normal "))==0) Mode=ModeNormal; 
	else if (_tcscmp(value, _T("2"))==0 || _tcscmp(value, _T("bombard "))==0)
	{
			BombardGet = BombardGetPayload = BombardPost = BombardPostReply = BombardPostReplyPayload = true;
	}
	else
	{
		BombardGet = _tcsstr(value, "/get")!=NULL;
		BombardGetPayload = _tcsstr(value, "/getpayload ")!=NULL;
		BombardPost = _tcsstr(value, "/post ")!=NULL;
		BombardPostReply = _tcsstr(value, "/postreply ")!=NULL;
		BombardPostReplyPayload = _tcsstr(value, "/postreplypayload ")!=NULL;
	}
	
	if (BombardGet || BombardGetPayload || BombardPost || BombardPostReply || BombardPostReplyPayload) 
		Mode = ModeBombard;
}
