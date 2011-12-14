#include "StdAfx.h"
#include "BarbaServerHttpCourier.h"


BarbaServerHttpCourier::BarbaServerHttpCourier(int maxConnection)
	: BarbaCourierServer(maxConnection)
{
}


BarbaServerHttpCourier::~BarbaServerHttpCourier(void)
{
}

void BarbaServerHttpCourier::Receive(BYTE* buffer, size_t bufferCount)
{
}

