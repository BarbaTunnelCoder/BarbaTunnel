#include "StdAfx.h"
#include "BarbaClient\BarbaClientApp.h"
#include "BarbaServer\BarbaServerApp.h"
#include "BarbaPacketFilter.h"
extern CNdisApi api;

ULARGE_INTEGER BarbaPacketFilter::GetAdapterHandle()
{
	ULARGE_INTEGER ret = {0};
	ret.LowPart = (DWORD)theApp->GetAdapterHandle();
	return ret;
}

bool BarbaPacketFilter::ApplyFilters(std::vector<STATIC_FILTER>* filters)
{
	size_t filtersBufSize = sizeof (STATIC_FILTER)*filters->size();
	std::vector<BYTE> filterTableBuf( sizeof STATIC_FILTER_TABLE +  filtersBufSize );
	STATIC_FILTER_TABLE* filterTable = (STATIC_FILTER_TABLE*)&filterTableBuf.front();
	filterTable->m_TableSize = filters->size();
	memcpy_s(filterTable->m_StaticFilters, filtersBufSize, filters->data(), filtersBufSize);
	return api.SetPacketFilterTable(filterTable)!=FALSE;
}

void BarbaPacketFilter::GetBypassPacketFilter(std::vector<STATIC_FILTER>* filters)
{
	//pass all other packets
	STATIC_FILTER staticFilter = {0};
	staticFilter.m_Adapter = GetAdapterHandle();
	staticFilter.m_FilterAction = FILTER_PACKET_PASS;
	staticFilter.m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE | PACKET_FLAG_ON_SEND;
	staticFilter.m_ValidFields = 0;

	filters->push_back(staticFilter);
}

void BarbaPacketFilter::GetFilter(STATIC_FILTER* staticFilter, bool send, u_long ipStart, u_long ipEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd)
{
	if (ipEnd==0) ipEnd = ipStart;
	if (srcPortEnd==0) srcPortEnd = srcPortStart;
	if (desPortEnd==0) desPortEnd = desPortStart;

	staticFilter->m_Adapter = GetAdapterHandle();
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

void BarbaPacketFilter::AddFilter(std::vector<STATIC_FILTER>* filters, bool send, u_long ipStart, u_long ipEnd, u_char protocol, u_short srcPortStart, u_short srcPortEnd, u_short desPortStart, u_short desPortEnd)
{
	STATIC_FILTER staticFilter = {0};
	GetFilter(&staticFilter, send, ipStart, ipEnd, protocol, srcPortStart, srcPortEnd, desPortStart, desPortEnd);
	filters->push_back(staticFilter);
}

void BarbaPacketFilter::GetClientFilters(std::vector<STATIC_FILTER>* filters, BarbaClientConfig* config)
{
	for (size_t i=0; i<config->Items.size(); i++)
	{
		BarbaClientConfigItem* item = &config->Items[i];
		if (!item->Enabled)
			continue;

		//filter only required packet going to server
		for (size_t i=0; i<item->GrabProtocols.size(); i++)
			AddFilter(filters, true, config->ServerIp, 0, item->GrabProtocols[i].Protocol, 0, 0, item->GrabProtocols[i].Port, 0);

		//redirect port
		if (item->RealPort!=0)
			AddFilter(filters, true, config->ServerIp, 0, item->GetTunnelProtocol(), 0, 0, item->RealPort, 0);

		//filter only tunnel packet that come from server except http-tunnel that use socket
		if (item->Mode!=BarbaModeHttpTunnel) 
			for (size_t i=0; i<item->TunnelPorts.size(); i++)
				AddFilter(filters, false, config->ServerIp, 0, item->GetTunnelProtocol(), item->TunnelPorts[i].StartPort, item->TunnelPorts[i].EndPort, 0, 0);
	}
}

void BarbaPacketFilter::GetServerFilters(std::vector<STATIC_FILTER>* filters, BarbaServerConfig* config)
{
	//filter incoming tunnel
	for (size_t i=0; i<config->Items.size(); i++)
	{
		BarbaServerConfigItem* item = &config->Items[i];
		if (!item->Enabled)
			continue;

		//filter only tunnel packet that come from server except http-tunnel that use socket
		if (item->Mode!=BarbaModeHttpTunnel) 
			for (size_t i=0; i<item->TunnelPorts.size(); i++)
				AddFilter(filters, false, 0, 0, item->GetTunnelProtocol(), 0, 0, item->TunnelPorts[i].StartPort, item->TunnelPorts[i].EndPort);
	}

	//filter outgoing virtual IP
	AddFilter(filters, true, theServerApp->VirtualIpRange.StartIp, theServerApp->VirtualIpRange.EndIp, 0, 0, 0, 0, 0);

	//filter ICMP for debug mode
	if (theApp->IsDebugMode())
		AddFilter(filters, true, 0, 0, IPPROTO_IGMP, 0, 0, 0, 0);
}


void BarbaPacketFilter::ApplyClientPacketFilter()
{
	std::vector<STATIC_FILTER> filters;
	for (size_t i=0; i<theClientApp->ConfigManager.Configs.size(); i++)
	{
		BarbaClientConfig* config = &theClientApp->ConfigManager.Configs[i];
		GetClientFilters(&filters, config);
	}
	
	//bypass all other packet
	GetBypassPacketFilter(&filters);

	//apply filters
	if (!ApplyFilters(&filters))
		throw new BarbaException(_T("Could not set packet filtering!"));
}

void BarbaPacketFilter::ApplyServerPacketFilter()
{
	std::vector<STATIC_FILTER> filters;
	GetServerFilters(&filters, &theServerApp->Config);

	//bypass all other packet
	GetBypassPacketFilter(&filters);

	//apply filters
	if (!ApplyFilters(&filters))
		throw new BarbaException(_T("Could not set packet filtering!"));
}

bool BarbaPacketFilter::ApplyPacketFilter()
{
	try
	{
		return true;
		if (theApp->IsServerMode())
			ApplyServerPacketFilter();
		else
			ApplyClientPacketFilter();
		return true;
	}
	catch(BarbaException* e)
	{
		BarbaLog(_T("Error: %s"), e->ToString());
		BarbaNotify(_T("Error: %s"), e->ToString());
		delete e;
		return false;
	}
}
