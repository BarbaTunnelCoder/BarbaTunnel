#pragma once
#include "barbafilterdriver.h"

class WinDivertFilterDriver :
	public BarbaFilterDriver
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

private:
	volatile HANDLE DivertHandle;
	volatile UINT32 MainIfIdx; // Packet's interface index.
    volatile UINT32 MainSubIfIdx; // Packet's sub-interface index.

};

