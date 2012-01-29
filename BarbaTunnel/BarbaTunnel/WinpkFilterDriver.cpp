#include "StdAfx.h"
#include "BarbaApp.h"
#include "WinpkFilterDriver.h"
#include "WinpkFilter\ndisapi.h"

TCP_AdapterList		AdList;
CNdisApi			api;

WinpkFilterDriver::WinpkFilterDriver()
{
	this->AdapterHandle = NULL;
	this->AdapterIndex = 0;
}

ULARGE_INTEGER WinpkFilterDriver::GetAdapterHandleLarge()
{
	ULARGE_INTEGER ret = {0};
	ret.QuadPart = (ULONGLONG)this->AdapterHandle;
	return ret;
}

bool WinpkFilterDriver::ApplyFilters(std::vector<STATIC_FILTER>* filters)
{
	size_t filtersBufSize = sizeof (STATIC_FILTER)*filters->size();
	std::vector<BYTE> filterTableBuf( sizeof STATIC_FILTER_TABLE +  filtersBufSize );
	STATIC_FILTER_TABLE* filterTable = (STATIC_FILTER_TABLE*)&filterTableBuf.front();
	filterTable->m_TableSize = (u_long)filters->size();
	memcpy_s(filterTable->m_StaticFilters, filtersBufSize, filters->data(), filtersBufSize);
	return api.SetPacketFilterTable(filterTable)!=FALSE;
}

void WinpkFilterDriver::AddBypassPacketFilter(std::vector<STATIC_FILTER>* filters)
{
	//pass all other packets
	STATIC_FILTER staticFilter = {0};
	staticFilter.m_Adapter = GetAdapterHandleLarge();
	staticFilter.m_FilterAction = FILTER_PACKET_PASS;
	staticFilter.m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE | PACKET_FLAG_ON_SEND;
	staticFilter.m_ValidFields = 0;

	filters->push_back(staticFilter);
}

void WinpkFilterDriver::ApplyPacketFilter()
{
	//collect filters
	std::vector<STATIC_FILTER> filters;
	AddPacketFilter(&filters);

	//WinpkFilter need to add bypass filter otherwise all other packets will be dropped
	AddBypassPacketFilter(&filters);

	//apply filters
	if (!ApplyFilters(&filters))
		throw new BarbaException(_T("Could not set packet filtering!"));
}

void WinpkFilterDriver::GetFilter(STATIC_FILTER* staticFilter, bool send, u_long ipStart, u_long ipEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd)
{
	if (ipEnd==0) ipEnd = ipStart;
	if (srcPortEnd==0) srcPortEnd = srcPortStart;
	if (desPortEnd==0) desPortEnd = desPortStart;

	staticFilter->m_Adapter = GetAdapterHandleLarge();
	staticFilter->m_FilterAction = FILTER_PACKET_REDIRECT;
	staticFilter->m_dwDirectionFlags = send ? PACKET_FLAG_ON_SEND : PACKET_FLAG_ON_RECEIVE;

	//ip filter
	if (ipStart!=0)
	{
		staticFilter->m_ValidFields |= NETWORK_LAYER_VALID;
		staticFilter->m_NetworkFilter.m_dwUnionSelector = IPV4;
		if (send)
		{
			staticFilter->m_NetworkFilter.m_IPv4.m_ValidFields |= IP_V4_FILTER_DEST_ADDRESS;
			staticFilter->m_NetworkFilter.m_IPv4.m_DestAddress.m_AddressType=IP_RANGE_V4_TYPE;
			staticFilter->m_NetworkFilter.m_IPv4.m_DestAddress.m_IpRange.m_StartIp = ipStart;
			staticFilter->m_NetworkFilter.m_IPv4.m_DestAddress.m_IpRange.m_EndIp = ipEnd;
		}
		else
		{
			staticFilter->m_NetworkFilter.m_IPv4.m_ValidFields |= IP_V4_FILTER_SRC_ADDRESS;
			staticFilter->m_NetworkFilter.m_IPv4.m_SrcAddress.m_AddressType=IP_RANGE_V4_TYPE;
			staticFilter->m_NetworkFilter.m_IPv4.m_SrcAddress.m_IpRange.m_StartIp = ipStart;
			staticFilter->m_NetworkFilter.m_IPv4.m_SrcAddress.m_IpRange.m_EndIp = ipEnd;
		}
	}

	//protocol filter
	if (protocol!=0)
	{
		staticFilter->m_ValidFields |= NETWORK_LAYER_VALID;
		staticFilter->m_NetworkFilter.m_dwUnionSelector = IPV4;
		staticFilter->m_NetworkFilter.m_IPv4.m_ValidFields |= IP_V4_FILTER_PROTOCOL;
		staticFilter->m_NetworkFilter.m_IPv4.m_Protocol = protocol;
	}

	//source port filter
	if (srcPortStart!=0 && (protocol==IPPROTO_TCP || protocol==IPPROTO_UDP))
	{
		staticFilter->m_ValidFields |= TRANSPORT_LAYER_VALID;
		staticFilter->m_TransportFilter.m_dwUnionSelector = TCPUDP;
		staticFilter->m_TransportFilter.m_TcpUdp.m_ValidFields |= TCPUDP_SRC_PORT;
		staticFilter->m_TransportFilter.m_TcpUdp.m_SourcePort.m_StartRange = srcPortStart;
		staticFilter->m_TransportFilter.m_TcpUdp.m_SourcePort.m_EndRange = srcPortEnd;
	}

	//destination port filter
	if (desPortStart!=0 && (protocol==IPPROTO_TCP || protocol==IPPROTO_UDP))
	{
		staticFilter->m_ValidFields |= TRANSPORT_LAYER_VALID;
		staticFilter->m_TransportFilter.m_dwUnionSelector = TCPUDP;
		staticFilter->m_TransportFilter.m_TcpUdp.m_ValidFields |= TCPUDP_DEST_PORT;
		staticFilter->m_TransportFilter.m_TcpUdp.m_DestPort.m_StartRange = desPortStart;
		staticFilter->m_TransportFilter.m_TcpUdp.m_DestPort.m_EndRange = desPortEnd;
	}
}

void WinpkFilterDriver::AddFilter(void* filter, bool send, u_long ipStart, u_long ipEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd)
{
	std::vector<STATIC_FILTER>* filters = (std::vector<STATIC_FILTER>*)filter;

	STATIC_FILTER staticFilter = {0};
	GetFilter(&staticFilter, send, ipStart, ipEnd, protocol, srcPortStart, srcPortEnd, desPortStart, desPortEnd);
	filters->push_back(staticFilter);
}


size_t WinpkFilterDriver::FindAdapterIndex()
{
	//get adapter list
	if (!api.GetTcpipBoundAdaptersInfo ( &AdList ) )
		throw new BarbaException(_T("WinpkFilter could not get Could not call GetTcpipBoundAdaptersInfo!"));

	//check is at least 1 adapter exists
	if ( AdList.m_nAdapterCount==0 )
		throw new BarbaException(_T("WinpkFilter could not get Could not find any network adapter!"));

	//use app adapter index if valid
	if (theApp->GetAdapterIndex()>0 && theApp->GetAdapterIndex()<=AdList.m_nAdapterCount)
		return theApp->GetAdapterIndex()-1;

	//find best adapter
	TCHAR msg[ADAPTER_NAME_SIZE*ADAPTER_LIST_SIZE + 255] = {0};
	_tcscat_s(msg, _T("Could not find main network adapter!\r\nPlease set your main network adapter index in AdapterIndex of BarbaTunnel.ini file.\r\n\r\n"));

	size_t findCount = 0;
	size_t findIndex = 0;
	for (size_t i=0; i<AdList.m_nAdapterCount; i++)
	{
		TCHAR adapterName[ADAPTER_NAME_SIZE];
		CNdisApi::ConvertWindows2000AdapterName((LPCTSTR)AdList.m_szAdapterNameList[i], adapterName, _countof(adapterName));

		TCHAR adapterNameLower[ADAPTER_NAME_SIZE] = {0};
		for(size_t j = 0; adapterName[j] != '\0' && i<ADAPTER_NAME_SIZE; j++)
			adapterNameLower[j] = (TCHAR)_totlower(adapterName[j]);

		bool isWan = _tcsstr(adapterNameLower, _T("wan network"))!=NULL;
		if (isWan)
			continue;

		//save last find
		findCount++;
		findIndex = i;

		//add adapter name
		TCHAR adapterLine[ADAPTER_NAME_SIZE + 10];
		_stprintf_s(adapterLine, _T("%d) %s"), i+1, adapterName);
		_tcscat_s(msg, adapterLine);
		_tcscat_s(msg, _T("\n"));
	}

	//use the only found adapter
	if (findCount==1)
		return findIndex;

	BarbaNotify(_T("Error: Could not find main network adapter!"));
	throw new BarbaException(msg);
}

bool WinpkFilterDriver::SendPacketToInbound(PacketHelper* packet)
{
	INTERMEDIATE_BUFFER intBuf = {0};
	intBuf.m_dwDeviceFlags = PACKET_FLAG_ON_RECEIVE;
	intBuf.m_Length = min(MAX_ETHER_FRAME, (ULONG)packet->GetPacketLen());
	memcpy_s(intBuf.m_IBuffer, MAX_ETHER_FRAME, packet->GetPacket(), intBuf.m_Length);

	ETH_REQUEST req;
	req.hAdapterHandle = this->AdapterHandle;
	req.EthPacket.Buffer = &intBuf;
	return api.SendPacketToMstcp(&req)!=FALSE;
}

bool WinpkFilterDriver::SendPacketToOutbound(PacketHelper* packet)
{
	INTERMEDIATE_BUFFER intBuf = {0};
	intBuf.m_dwDeviceFlags = PACKET_FLAG_ON_SEND;
	intBuf.m_Length = min(MAX_ETHER_FRAME, (ULONG)packet->GetPacketLen());
	memcpy_s(intBuf.m_IBuffer, MAX_ETHER_FRAME, packet->GetPacket(), intBuf.m_Length);

	ETH_REQUEST req;
	req.hAdapterHandle = this->AdapterHandle;
	req.EthPacket.Buffer = &intBuf;
	return api.SendPacketToAdapter(&req)!=FALSE;
}

void WinpkFilterDriver::Dispose()
{
	// This function releases packets in the adapter queue and stops listening the interface
	BarbaFilterDriver::Dispose();

	// Set NULL event to release previously set event object
	api.SetPacketEvent(this->AdapterHandle, NULL);

	// Set default adapter mode
	ADAPTER_MODE Mode;
	Mode.dwFlags = 0;
	Mode.hAdapterHandle = this->AdapterHandle;
	api.SetAdapterMode(&Mode);

	// Empty adapter packets queue
	api.FlushAdapterPacketQueue(this->AdapterHandle);
	Mode.hAdapterHandle = NULL;
}

void WinpkFilterDriver::Initialize()
{
	//check is driver loaded (let after Comm Files created)
	if(!api.IsDriverLoaded())
	{
		LPCTSTR err = _T("Driver not installed on this system or failed to load!\r\nPlease go to http://www.ntndis.com/w&p.php?id=7 and install WinpkFilter driver.");
		BarbaNotify(_T("Error: Driver not installed!\r\nDriver not installed on this system or failed to load!"));
		throw new BarbaException(err);
	}

	//FindAdapterIndex
	this->AdapterIndex = FindAdapterIndex();
	this->AdapterHandle = AdList.m_nAdapterHandle[this->AdapterIndex];

	//try to set MTU
	UpdateMTUDecrement();

	//Initialize
	ADAPTER_MODE adapterMode;
	adapterMode.dwFlags = MSTCP_FLAG_SENT_TUNNEL | MSTCP_FLAG_RECV_TUNNEL;
	adapterMode.hAdapterHandle = this->AdapterHandle;
	api.SetAdapterMode(&adapterMode);
	ApplyPacketFilter();

	// Create notification event
	this->WinpkPacketEvent.Attach( CreateEvent(NULL, TRUE, FALSE, NULL) );
	
	// Set event for helper driver
	if (!api.SetPacketEvent(adapterMode.hAdapterHandle, this->WinpkPacketEvent.GetHandle()))
		throw new BarbaException(_T("Failed to create notification event or set it for driver."));
}

void WinpkFilterDriver::StartCaptureLoop()
{
	// Initialize Request
	ETH_REQUEST request = {0};
	INTERMEDIATE_BUFFER CurrentPacketBuffer = {0};
	request.hAdapterHandle = this->AdapterHandle;
	request.EthPacket.Buffer = &CurrentPacketBuffer;

	//Handle
	HANDLE events[2];
	events[0] = this->WinpkPacketEvent.GetHandle();
	events[1] = this->StopEvent.GetHandle();

	while (!this->StopEvent.IsSet())
	{
		DWORD res = WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
		if (res==WAIT_OBJECT_0-0)
		{
			while(api.ReadPacket(&request))
			{
				PINTERMEDIATE_BUFFER buffer = request.EthPacket.Buffer;
				bool send = buffer->m_dwDeviceFlags == PACKET_FLAG_ON_SEND;
				PacketHelper* packet = new PacketHelper((ether_header_ptr)buffer->m_IBuffer);
				AddPacket(packet, send);
			}
			WinpkPacketEvent.Reset();
		}
	}
}

DWORD WinpkFilterDriver::GetMTUDecrement()
{
	return api.GetMTUDecrement();
}

void WinpkFilterDriver::SetMTUDecrement(DWORD value)
{
	if (!api.SetMTUDecrement(value))
		throw new BarbaException(_T("WinpkFilter could not set MTUDecrement to %d!"), value);
}
