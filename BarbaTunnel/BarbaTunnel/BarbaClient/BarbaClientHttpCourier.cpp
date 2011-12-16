#include "StdAfx.h"
#include "BarbaClientHttpCourier.h"


BarbaClientHttpCourier::BarbaClientHttpCourier(DWORD remoteIp, u_short remotePort, u_short maxConnenction)
	: BarbaCourierClient(remoteIp, remotePort, maxConnenction)
{
}


BarbaClientHttpCourier::~BarbaClientHttpCourier(void)
{
}
