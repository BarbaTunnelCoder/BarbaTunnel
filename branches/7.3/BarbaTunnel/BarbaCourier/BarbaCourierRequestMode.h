#pragma once
#include "General.h"

class BarbaCourierRequestMode
{
public:
	enum ModeEnum
	{
		ModeNone,
		ModeNormal,
		ModeBombard,
	};

private:
	void Reset();

public:
	BarbaCourierRequestMode();
	std::tstring ToString();
	void Parse(std::tstring value);
	ModeEnum Mode;
	bool IsFullBombard();
	bool BombardGet;
	bool BombardGetPayload;
	bool BombardPost;
	bool BombardPostReply;
	bool BombardPostReplyPayload;
};
