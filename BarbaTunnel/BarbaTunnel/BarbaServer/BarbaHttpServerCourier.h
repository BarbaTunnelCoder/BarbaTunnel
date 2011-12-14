/*
 * BarbaHttpServerCourier class
 * this class just used for HTTP-Tunneling
 */

#pragma once
#include "BarbaCourier.h"

class BarbaHttpServerCourier : public BarbaCourierServer
{
public:
	explicit BarbaHttpServerCourier(int maxConnection);
	virtual ~BarbaHttpServerCourier(void);
	GUID SessionID;

	virtual void Receive(BYTE* buffer, size_t bufferCount);

};

