#pragma once
#include "BarbaCourierDatagram.h"
class BarbaCourierUdpServer : public BarbaCourierDatagram
{
public:
	struct CreateStrcutUdp : public CreateStrcut
	{
	};

public:
	BarbaCourierUdpServer(CreateStrcutUdp* cs);
	virtual ~BarbaCourierUdpServer(void);
};

