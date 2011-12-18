/*
 * BarbaServerHttpCourier class
 * this class just used for HTTP-Tunneling
 */

#pragma once
#include "BarbaCourier.h"

class BarbaServerHttpCourier : public BarbaCourierServer
{
public:
	explicit BarbaServerHttpCourier(u_short maxConnection);
	virtual void Receive(BYTE* buffer, size_t bufferCount);
	GUID SessionID;


protected:
	virtual ~BarbaServerHttpCourier(void);
};

