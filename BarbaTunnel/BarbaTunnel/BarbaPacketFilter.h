#pragma once

class BarbaPacketFilter
{
public:
	explicit BarbaPacketFilter(void);
	virtual ~BarbaPacketFilter(void);
	//@return false when job finished and ready to dispose
	virtual bool ReadPacket(PacketHelper* packet, bool* send)=0;
	virtual void Dispose()=0;
	virtual void Start()=0;
	virtual void Stop()=0;
	virtual LPCTSTR GetName()=0;
	virtual bool SendPacketToOutbound(PacketHelper* packet)=0;
	virtual bool SendPacketToInbound(PacketHelper* packet)=0;
};

