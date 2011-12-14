#include "StdAfx.h"
#include "BarbaHttpServerCourier.h"


BarbaHttpServerCourier::BarbaHttpServerCourier(int maxConnection)
	: BarbaCourierServer(maxConnection)
{
}


BarbaHttpServerCourier::~BarbaHttpServerCourier(void)
{
}

void BarbaHttpServerCourier::Receive(BYTE* buffer, size_t bufferCount)
{
}

