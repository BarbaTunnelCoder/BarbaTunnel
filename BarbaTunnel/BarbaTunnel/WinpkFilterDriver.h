#pragma once
#include "General.h"
#include "BarbaFilterDriver.h"
#include "BarbaClient\BarbaClientConfig.h"
#include "BarbaServer\BarbaServerConfig.h"

class WinpkFilterDriver : public BarbaFilterDriver
{
public:
	explicit WinpkFilterDriver();
	virtual void Initialize();
	virtual void StartCaptureLoop();
	virtual void Dispose();
	virtual bool SendPacketToOutbound(PacketHelper* packet);
	virtual bool SendPacketToInbound(PacketHelper* packet);
	virtual NetworkLayerEnum GetNetworkLayer() {return NetworkLayerDateLink;}
	virtual DWORD GetMTUDecrement();
	virtual void SetMTUDecrement(DWORD value);
	virtual LPCTSTR GetName() {return _T("WinpkFilter");}

private: 
	//Filtering methods
	void ApplyPacketFilter();
	void ApplyClientPacketFilter();
	void ApplyServerPacketFilter();
	bool ApplyFilters(std::vector<STATIC_FILTER>* filters);
	void GetClientFilters(std::vector<STATIC_FILTER>* filters, std::vector<BarbaClientConfig>* configs);
	void GetServerFilters(std::vector<STATIC_FILTER>* filters, std::vector<BarbaServerConfig>* configs);
	void GetBypassPacketFilter(std::vector<STATIC_FILTER>* filters);
	void GetFilter(STATIC_FILTER* staticFilter, bool send, u_long ipStart, u_long ipEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd);
	void AddFilter(std::vector<STATIC_FILTER>* filters, bool send, u_long ipStart, u_long ipEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd);
	
	//Process Methods
	ULARGE_INTEGER GetAdapterHandleLarge();
	size_t FindAdapterIndex();
	size_t AdapterIndex;
	SimpleEvent WinpkPacketEvent;
	HANDLE AdapterHandle;
};

