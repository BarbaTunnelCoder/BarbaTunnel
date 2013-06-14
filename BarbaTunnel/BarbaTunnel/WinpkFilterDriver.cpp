#include "StdAfx.h"
#include "BarbaApp.h"
#include "WinpkFilterDriver.h"
#include "WinpkFilter\WinpkFilterApi.h"

TCP_AdapterList		AdList;
WinpkFilterApi gWinpkFilterApi;

void InitWinpkFilterApi()
{
	if (gWinpkFilterApi.ModuleHandle!=NULL)
		return;

	//initialize DivertModule
	TCHAR curDir[MAX_PATH];
	GetCurrentDirectory(_countof(curDir), curDir);
	SetCurrentDirectory( BarbaApp::GetModuleFolder() ); //WinDivert need current directory to install driver perhaps for WdfCoInstaller01009
	HMODULE module = LoadLibrary(_T("ndisapi.dll"));
	SetCurrentDirectory(curDir);
	if (module==NULL) 
		throw _T("Could not load ndisapi.dll module!");
	gWinpkFilterApi.Init(module);
}

WinpkFilterDriver::WinpkFilterDriver()
{
	AdapterHandle = NULL;
	AdapterIndex = 0;
	FilterDriverHandle = NULL;
	memset(&OutboundAddress, sizeof OutboundAddress, 0);
	memset(&InboundAddress, sizeof InboundAddress, 0);
}

ULARGE_INTEGER WinpkFilterDriver::GetAdapterHandleLarge()
{
	ULARGE_INTEGER ret = {0};
	ret.QuadPart = (ULONGLONG)AdapterHandle;
	return ret;
}

bool WinpkFilterDriver::ApplyFilters(std::vector<STATIC_FILTER>* filters)
{
	size_t filtersBufSize = sizeof (STATIC_FILTER)*filters->size();
	BarbaBuffer filterTableBuf( sizeof STATIC_FILTER_TABLE +  filtersBufSize );
	STATIC_FILTER_TABLE* filterTable = (STATIC_FILTER_TABLE*)filterTableBuf.data();
	filterTable->m_TableSize = (u_long)filters->size();
	memcpy_s(filterTable->m_StaticFilters, filtersBufSize, filters->data(), filtersBufSize);
	return gWinpkFilterApi.SetPacketFilterTable(FilterDriverHandle, filterTable)!=FALSE;
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

void WinpkFilterDriver::GetFilter(STATIC_FILTER* staticFilter, bool send, u_long srcIpStart, u_long srcIpEnd, u_long desIpStart, u_long desIpEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd)
{
	srcIpEnd = max(srcIpStart, srcIpEnd);
	desIpEnd = max(desIpStart, desIpEnd);
	srcPortEnd = max(srcPortStart, srcPortEnd);
	desPortEnd = max(desPortStart, desPortEnd);

	staticFilter->m_Adapter = GetAdapterHandleLarge();
	staticFilter->m_FilterAction = FILTER_PACKET_REDIRECT;
	staticFilter->m_dwDirectionFlags = send ? PACKET_FLAG_ON_SEND : PACKET_FLAG_ON_RECEIVE;

	//ip filter
	if (srcIpStart!=0)
	{
		staticFilter->m_ValidFields |= NETWORK_LAYER_VALID;
		staticFilter->m_NetworkFilter.m_dwUnionSelector = IPV4;
		staticFilter->m_NetworkFilter.m_IPv4.m_ValidFields |= IP_V4_FILTER_SRC_ADDRESS;
		staticFilter->m_NetworkFilter.m_IPv4.m_SrcAddress.m_AddressType=IP_RANGE_V4_TYPE;
		staticFilter->m_NetworkFilter.m_IPv4.m_SrcAddress.m_IpRange.m_StartIp = srcIpStart;
		staticFilter->m_NetworkFilter.m_IPv4.m_SrcAddress.m_IpRange.m_EndIp = srcIpEnd;
	}

	if (desIpStart!=0)
	{
		staticFilter->m_ValidFields |= NETWORK_LAYER_VALID;
		staticFilter->m_NetworkFilter.m_dwUnionSelector = IPV4;
		staticFilter->m_NetworkFilter.m_IPv4.m_ValidFields |= IP_V4_FILTER_DEST_ADDRESS;
		staticFilter->m_NetworkFilter.m_IPv4.m_DestAddress.m_AddressType=IP_RANGE_V4_TYPE;
		staticFilter->m_NetworkFilter.m_IPv4.m_DestAddress.m_IpRange.m_StartIp = desIpStart;
		staticFilter->m_NetworkFilter.m_IPv4.m_DestAddress.m_IpRange.m_EndIp = desIpEnd;
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

void WinpkFilterDriver::AddFilter(void* filter, bool send, u_long srcIpStart, u_long srcIpEnd, u_long desIpStart, u_long desIpEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd)
{
	std::vector<STATIC_FILTER>* filters = (std::vector<STATIC_FILTER>*)filter;

	STATIC_FILTER staticFilter = {0};
	GetFilter(&staticFilter, send, srcIpStart, srcIpEnd, desIpStart, desIpEnd, protocol, srcPortStart, srcPortEnd, desPortStart, desPortEnd);
	filters->push_back(staticFilter);
}

void WinpkFilterDriver::GetBestInternetAdapter(std::string* adapterName, BarbaBuffer* address)
{
	//Get System Adapter infos
	ULONG size = 0;
	GetAdaptersInfo(NULL, &size);
	IP_ADAPTER_INFO* adapterInfos = (IP_ADAPTER_INFO*)new BYTE[size];
	if (GetAdaptersInfo(adapterInfos, &size)!=NO_ERROR)
	{
		delete adapterInfos;
		return;
	}
	
	DWORD bestIfaceIndex = 0;
	if (GetBestInterface( inet_addr("8.8.8.8"), &bestIfaceIndex)!=NO_ERROR)
	{
		delete adapterInfos;
		return;
	}
	
	IP_ADAPTER_INFO* info = adapterInfos;
	while (info!=NULL)
	{
		if (info->ComboIndex==bestIfaceIndex)
		{
			adapterName->append(info->AdapterName);
			address->assign(info->Address, info->AddressLength);
			break;
		}
		info = info->Next;
	}
	delete adapterInfos;
}


size_t WinpkFilterDriver::FindAdapterIndex()
{
	//get adapter list
	if (!gWinpkFilterApi.GetTcpipBoundAdaptersInfo (FilterDriverHandle,  &AdList ) )
		throw new BarbaException(_T("WinpkFilter could not get Could not call GetTcpipBoundAdaptersInfo!"));

	//check is at least 1 adapter exists
	if ( AdList.m_nAdapterCount==0 )
		throw new BarbaException(_T("WinpkFilter could not get Could not find any network adapter!"));

	//use app adapter index if valid
	if (theApp->GetAdapterIndex()>0 && theApp->GetAdapterIndex()<=AdList.m_nAdapterCount)
		return theApp->GetAdapterIndex()-1;


	//find best adapter
	std::string bestAdapaterName;
	BarbaBuffer bestAdapaterAddress;
	GetBestInternetAdapter(&bestAdapaterName, &bestAdapaterAddress);
	StringUtils::MakeLower(bestAdapaterName);

	TCHAR msg[ADAPTER_NAME_SIZE*ADAPTER_LIST_SIZE + 255] = {0};
	_tcscat_s(msg, _T("Could not find main network adapter!\r\nPlease set your main network adapter index in AdapterIndex of BarbaTunnel.ini file.\r\n\r\n"));

	int findIndex = -1;
	for (int i=0; i<(int)AdList.m_nAdapterCount; i++)
	{
		CHAR internalNameLower[ADAPTER_NAME_SIZE] = {0};
		for(size_t j = 0; j<ADAPTER_NAME_SIZE && AdList.m_szAdapterNameList[i][j] != '\0'; j++)
			internalNameLower[j] = (CHAR)tolower((CHAR)AdList.m_szAdapterNameList[i][j]);

		//compare with best adapter
		if (!bestAdapaterName.empty() && strstr(internalNameLower, bestAdapaterName.data())!=NULL 
			&& bestAdapaterAddress.size()==ETHER_ADDR_LENGTH 
			&& memcpy_s(bestAdapaterAddress.data(), bestAdapaterAddress.size(), AdList.m_czCurrentAddress[i], ETHER_ADDR_LENGTH)==0)
		{
			findIndex = i;
			break;
		}

		//add adapter name
		CHAR adapterName[ADAPTER_NAME_SIZE];
		gWinpkFilterApi.ConvertWindows2000AdapterName((LPCSTR)AdList.m_szAdapterNameList[i], adapterName, _countof(adapterName));
		TCHAR adapterLine[ADAPTER_NAME_SIZE + 10];
		_stprintf_s(adapterLine, _T("%d) %s"), i+1, adapterName);
		_tcscat_s(msg, adapterLine);
		_tcscat_s(msg, _T("\n"));
	}

	//use the only found adapter
	if (findIndex!=-1)
		return findIndex;

	BarbaNotify(_T("Error: Could not find main network adapter!"));
	throw new BarbaException(msg);
}

bool WinpkFilterDriver::SendPacketToInbound(PacketHelper* packet)
{
	//fix ethernet packet
	memcpy_s(packet->ethHeader->h_source, ETH_ALEN, OutboundAddress, ETH_ALEN);
	memcpy_s(packet->ethHeader->h_dest, ETH_ALEN, InboundAddress, ETH_ALEN);

	INTERMEDIATE_BUFFER intBuf = {0};
	intBuf.m_dwDeviceFlags = PACKET_FLAG_ON_RECEIVE;
	intBuf.m_Length = min(MAX_ETHER_FRAME, (ULONG)packet->GetPacketLen());
	memcpy_s(intBuf.m_IBuffer, MAX_ETHER_FRAME, packet->GetPacket(), intBuf.m_Length);

	ETH_REQUEST req;
	req.hAdapterHandle = AdapterHandle;
	req.EthPacket.Buffer = &intBuf;
	return gWinpkFilterApi.SendPacketToMstcp(FilterDriverHandle, &req)!=FALSE;
}

bool WinpkFilterDriver::SendPacketToOutbound(PacketHelper* packet)
{
	//fix ethernet address
	memcpy_s(packet->ethHeader->h_source, ETH_ALEN, InboundAddress, ETH_ALEN);
	memcpy_s(packet->ethHeader->h_dest, ETH_ALEN, OutboundAddress, ETH_ALEN);

	INTERMEDIATE_BUFFER intBuf = {0};
	intBuf.m_dwDeviceFlags = PACKET_FLAG_ON_SEND;
	intBuf.m_Length = min(MAX_ETHER_FRAME, (ULONG)packet->GetPacketLen());
	memcpy_s(intBuf.m_IBuffer, MAX_ETHER_FRAME, packet->GetPacket(), intBuf.m_Length);

	ETH_REQUEST req;
	req.hAdapterHandle = AdapterHandle;
	req.EthPacket.Buffer = &intBuf;
	return gWinpkFilterApi.SendPacketToAdapter(FilterDriverHandle, &req)!=FALSE;
}

void WinpkFilterDriver::Dispose()
{
	//WARNING: Dispose parent first and wait till its thread closes
	BarbaFilterDriver::Dispose();

	// Set NULL event to release previously set event object
	gWinpkFilterApi.SetPacketEvent(FilterDriverHandle, AdapterHandle, NULL);

	// Set default adapter mode
	ADAPTER_MODE Mode;
	Mode.dwFlags = 0;
	Mode.hAdapterHandle = AdapterHandle;
	gWinpkFilterApi.SetAdapterMode(FilterDriverHandle, &Mode);

	// Empty adapter packets queue
	gWinpkFilterApi.FlushAdapterPacketQueue(FilterDriverHandle, AdapterHandle);
	Mode.hAdapterHandle = NULL;

	//close driver handle
	if (FilterDriverHandle!=NULL)
		gWinpkFilterApi.CloseFilterDriver(FilterDriverHandle);
}

void WinpkFilterDriver::Initialize()
{
	InitWinpkFilterApi();
	FilterDriverHandle = gWinpkFilterApi.OpenFilterDriver(DRIVER_NAME_A);
	if (FilterDriverHandle==NULL)
		throw new BarbaException(_T("Could not open WinpkFilter handle!"));

	//check is driver loaded (let after Comm Files created)
	if(!gWinpkFilterApi.IsDriverLoaded(FilterDriverHandle))
	{
		LPCTSTR err = _T("Driver not installed on this system or failed to load!\r\nPlease go to http://www.ntkernel.com/ and install WinpkFilter driver.");
		BarbaNotify(_T("Error: Driver not installed!\r\nDriver not installed on this system or failed to load!"));
		throw new BarbaException(err);
	}

	//FindAdapterIndex
	AdapterIndex = FindAdapterIndex();
	AdapterHandle = AdList.m_nAdapterHandle[AdapterIndex];

	//report info
	TCHAR adapterName[ADAPTER_NAME_SIZE];
	gWinpkFilterApi.ConvertWindows2000AdapterName((LPCTSTR)AdList.m_szAdapterNameList[AdapterIndex], adapterName, _countof(adapterName));
	BarbaLog(_T("Adapter: %s"), adapterName);

	//try to set MTU
	UpdateMTUDecrement();

	//Initialize
	ADAPTER_MODE adapterMode;
	adapterMode.dwFlags = MSTCP_FLAG_SENT_TUNNEL | MSTCP_FLAG_RECV_TUNNEL;
	adapterMode.hAdapterHandle = AdapterHandle;
	gWinpkFilterApi.SetAdapterMode(FilterDriverHandle, &adapterMode);
	ApplyPacketFilter();

	// Create notification event
	WinpkPacketEvent.Attach( CreateEvent(NULL, TRUE, FALSE, NULL) );
	
	// Set event for helper driver
	if (!gWinpkFilterApi.SetPacketEvent(FilterDriverHandle, adapterMode.hAdapterHandle, WinpkPacketEvent.GetHandle()))
		throw new BarbaException(_T("Failed to create notification event or set it for driver."));
}

void WinpkFilterDriver::StartCaptureLoop()
{
	// Initialize Request
	ETH_REQUEST request = {0};
	INTERMEDIATE_BUFFER CurrentPacketBuffer = {0};
	request.hAdapterHandle = AdapterHandle;
	request.EthPacket.Buffer = &CurrentPacketBuffer;

	//Handle
	HANDLE events[2];
	events[0] = WinpkPacketEvent.GetHandle();
	events[1] = StopEvent.GetHandle();

	while (!StopEvent.IsSet())
	{
		DWORD res = WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
		if (res==WAIT_OBJECT_0-0)
		{
			while(gWinpkFilterApi.ReadPacket(FilterDriverHandle, &request))
			{
				PINTERMEDIATE_BUFFER buffer = request.EthPacket.Buffer;
				bool send = buffer->m_dwDeviceFlags == PACKET_FLAG_ON_SEND;
				PacketHelper* packet = new PacketHelper((ether_header_ptr)buffer->m_IBuffer, buffer->m_Length);
				SaveEthernetHeader(packet, send);
				AddPacket(packet, send);
			}
			WinpkPacketEvent.Reset();
		}
	}
}

void WinpkFilterDriver::SaveEthernetHeader(PacketHelper* packet, bool outbound)
{
	static bool isAlreadySet = false;
	if (isAlreadySet)
		return;
	isAlreadySet = true;

	memcpy_s(OutboundAddress, ETH_ALEN, outbound ? packet->ethHeader->h_dest : packet->ethHeader->h_source, ETH_ALEN);
	memcpy_s(InboundAddress, ETH_ALEN,  outbound ? packet->ethHeader->h_source : packet->ethHeader->h_dest, ETH_ALEN);
}
	

DWORD WinpkFilterDriver::GetMTUDecrement()
{
	return gWinpkFilterApi.GetMTUDecrement();
}

void WinpkFilterDriver::SetMTUDecrement(DWORD value)
{
	if (!gWinpkFilterApi.SetMTUDecrement(value))
		throw new BarbaException(_T("WinpkFilter could not set MTUDecrement to %d!"), value);
}
