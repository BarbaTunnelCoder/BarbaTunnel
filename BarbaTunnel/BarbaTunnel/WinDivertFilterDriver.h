#pragma once
#include "barbafilterdriver.h"
#include "BarbaClient\BarbaClientConfig.h"
#include "BarbaServer\BarbaServerConfig.h"

class WinDivertFilterDriver : public BarbaFilterDriver
{
public:
	WinDivertFilterDriver(void);
	virtual ~WinDivertFilterDriver(void);

	virtual void Dispose();
	virtual void Initialize();
	virtual void StartCaptureLoop();
	virtual void Stop();
	virtual bool SendPacketToOutbound(PacketHelper* packet);
	virtual bool SendPacketToInbound(PacketHelper* packet);
	virtual DWORD GetMTUDecrement();
	virtual void SetMTUDecrement(DWORD value);
	virtual LPCTSTR GetName() {return _T("WinDivert");}
	virtual NetworkLayerEnum GetNetworkLayer() { return NetworkLayerTransport; }
	virtual void AddFilter(void* filter, bool send, u_long srcIpStart, u_long srcIpEnd, u_long desIpStart, u_long desIpEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd);

private:
	HANDLE OpenDivertHandle();
	void CreateRangeFormat(TCHAR* format, LPCSTR fieldName, DWORD start, DWORD end, bool ip=false);
	std::string GetFilter(bool send, u_long srcIpStart, u_long srcIpEnd, u_long desIpStart, u_long desIpEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd);
	volatile HANDLE DivertHandle;
	volatile UINT32 MainIfIdx; // Packet's interface index.
    volatile UINT32 MainSubIfIdx; // Packet's sub-interface index.
	bool FilterIpOnly; //used when filter exceed limit
};

