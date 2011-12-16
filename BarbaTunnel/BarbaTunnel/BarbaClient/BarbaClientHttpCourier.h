#pragma once
#include "BarbaCourier.h"

class BarbaClientHttpCourier :
	public BarbaCourierClient
{
public:
	explicit BarbaClientHttpCourier(DWORD remoteIp, u_short remotePort, u_short maxConnenction);
	virtual ~BarbaClientHttpCourier(void);
};

