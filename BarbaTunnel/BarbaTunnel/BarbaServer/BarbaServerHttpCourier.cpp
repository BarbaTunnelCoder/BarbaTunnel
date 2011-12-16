#include "StdAfx.h"
#include "BarbaServerHttpCourier.h"


BarbaServerHttpCourier::BarbaServerHttpCourier(u_short maxConnection)
	: BarbaCourierServer(maxConnection)
{
}


BarbaServerHttpCourier::~BarbaServerHttpCourier(void)
{
}

void BarbaServerHttpCourier::Receive(BYTE* buffer, size_t bufferCount)
{
}

