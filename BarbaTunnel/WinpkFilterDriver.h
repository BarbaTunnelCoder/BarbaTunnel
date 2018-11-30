#pragma once
#include "General.h"
#include "BarbaFilterDriver.h"

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
	virtual void AddFilter(void* filter, bool send, u_long srcIpStart, u_long srcIpEnd, u_long desIpStart, u_long desIpEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd);

private: 
	//Filtering methods
	void ApplyPacketFilter();
	bool ApplyFilters(std::vector<STATIC_FILTER>* filters);
	void AddBypassPacketFilter(std::vector<STATIC_FILTER>* filters);
	void GetFilter(STATIC_FILTER* staticFilter, bool send, u_long srcIpStart, u_long srcIpEnd, u_long desIpStart, u_long desIpEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd);
	//helper methid
	static void GetBestInternetAdapter(std::string* adapterName, BarbaBuffer* address);
	void SaveEthernetHeader(PacketHelper* packet, bool outbound);
	
	//Process Methods
	HANDLE FilterDriverHandle;
	ULARGE_INTEGER GetAdapterHandleLarge();
	size_t FindAdapterIndex();
	size_t AdapterIndex;
	SimpleEvent WinpkPacketEvent;
	HANDLE AdapterHandle;
	ether_header OutboundAddress[ETH_ALEN];
	ether_header InboundAddress[ETH_ALEN];
};

