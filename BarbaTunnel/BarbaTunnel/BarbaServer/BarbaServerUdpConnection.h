#pragma once
#include "BarbaServerConnection.h"

class BarbaServerUdpConnection :
	public BarbaServerConnection
{
public:
	BarbaServerUdpConnection(void);
	virtual ~BarbaServerUdpConnection(void);
};

