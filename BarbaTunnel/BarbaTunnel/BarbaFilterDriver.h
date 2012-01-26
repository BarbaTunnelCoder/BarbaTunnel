#pragma once
#include "General.h"

class BarbaFilterDriver
{
public:
	enum NetworkLayerEnum{
		NetworkLayerDateLink,
		NetworkLayerTransport,
	};

public:
	explicit BarbaFilterDriver(void);
	virtual ~BarbaFilterDriver(void);
	//@return false when job finished and ready to dispose
	virtual void Dispose()=0;
	virtual void Initialize()=0;
	virtual void Start()=0;
	virtual void Stop()=0;
	virtual LPCTSTR GetName()=0;
	virtual bool SendPacketToOutbound(PacketHelper* packet)=0;
	virtual bool SendPacketToInbound(PacketHelper* packet)=0;
	virtual NetworkLayerEnum GetNetworkLayer()=0;
	virtual DWORD GetMTUDecrement()=0;
	virtual void SetMTUDecrement(DWORD value)=0;
	virtual size_t GetMaxPacketLen()=0;
};

