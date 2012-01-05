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

void BarbaPacketFilter::GetClientFilters(std::vector<STATIC_FILTER>* filters, BarbaClientConfig* config)
{
	for (size_t i=0; i<config->Items.size(); i++)
	{
		BarbaClientConfigItem* item = &config->Items[i];
		if (item->Enabled)
		{
			//filter only required packet going to server
			GetClientGrabFilters(filters, config->ServerIp, &item->GrabProtocols);
			//filter only tunnel packet that come from server except http-tunnel that use socket
			//if (item->Mode!=BarbaModeHttpTunnel) 
				//GetClientTunnelFilters(filters, config->ServerIp, item->GetTunnelProtocol(), &item->TunnelPorts);
			//bypass all other packet
			GetBypassPacketFilter(filters);
		}
	}
}

void BarbaPacketFilter::GetFilter(STATIC_FILTER* staticFilter, bool send, u_long startIp, u_long endIp, u_char protocol, PortRange* srcPortRange, PortRange* desPortRange)
{
	staticFilter->m_Adapter = GetAdapterHandle();
	staticFilter->m_FilterAction = FILTER_PACKET_REDIRECT;
	staticFilter->m_dwDirectionFlags = send ? PACKET_FLAG_ON_SEND : PACKET_FLAG_ON_RECEIVE;

	//ip filter
	if (startIp!=0)
	{
		staticFilter->m_ValidFields |= NETWORK_LAYER_VALID;
		staticFilter->m_NetworkFilter.m_dwUnionSelector = IPV4;
		if (send)
		{
			staticFilter->m_NetworkFilter.m_IPv4.m_ValidFields |= IP_V4_FILTER_DEST_ADDRESS;
			staticFilter->m_NetworkFilter.m_IPv4.m_DestAddress.m_AddressType=IP_RANGE_V4_TYPE;
			staticFilter->m_NetworkFilter.m_IPv4.m_DestAddress.m_IpRange.m_StartIp = startIp;
			staticFilter->m_NetworkFilter.m_IPv4.m_DestAddress.m_IpRange.m_EndIp = endIp;
		}
		else
		{
			staticFilter->m_NetworkFilter.m_IPv4.m_ValidFields |= IP_V4_FILTER_SRC_ADDRESS;
			staticFilter->m_NetworkFilter.m_IPv4.m_SrcAddress.m_AddressType=IP_RANGE_V4_TYPE;
			staticFilter->m_NetworkFilter.m_IPv4.m_SrcAddress.m_IpRange.m_StartIp = startIp;
			staticFilter->m_NetworkFilter.m_IPv4.m_SrcAddress.m_IpRange.m_EndIp = endIp;
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
	if (srcPortRange!=NULL && (protocol==IPPROTO_TCP || protocol==IPPROTO_UDP))
	{
		staticFilter->m_ValidFields |= TRANSPORT_LAYER_VALID;
		staticFilter->m_TransportFilter.m_dwUnionSelector = TCPUDP;
		staticFilter->m_TransportFilter.m_TcpUdp.m_ValidFields |= TCPUDP_SRC_PORT;
		staticFilter->m_TransportFilter.m_TcpUdp.m_SourcePort.m_StartRange = srcPortRange->StartPort;
		staticFilter->m_TransportFilter.m_TcpUdp.m_SourcePort.m_EndRange = srcPortRange->EndPort;
	}

	//destination port filter
	if (desPortRange!=NULL && (protocol==IPPROTO_TCP || protocol==IPPROTO_UDP))
	{
		staticFilter->m_ValidFields |= TRANSPORT_LAYER_VALID;
		staticFilter->m_TransportFilter.m_dwUnionSelector = TCPUDP;
		staticFilter->m_TransportFilter.m_TcpUdp.m_ValidFields |= TCPUDP_DEST_PORT;
		staticFilter->m_TransportFilter.m_TcpUdp.m_DestPort.m_StartRange = desPortRange->StartPort;
		staticFilter->m_TransportFilter.m_TcpUdp.m_DestPort.m_EndRange = desPortRange->EndPort;
	}

}


void BarbaPacketFilter::GetClientGrabFilters(std::vector<STATIC_FILTER>* filters, u_long remoteIp, std::vector<ProtocolPort>* protocolPorts)
{
	for (size_t i=0; i<protocolPorts->size(); i++)
	{
		ProtocolPort* protocolPort = &protocolPorts->at(i);

		STATIC_FILTER staticFilter = {0};
		staticFilter.m_Adapter = GetAdapterHandle();
		staticFilter.m_FilterAction = FILTER_PACKET_REDIRECT;
		staticFilter.m_dwDirectionFlags = PACKET_FLAG_ON_SEND;
		staticFilter.m_ValidFields |= NETWORK_LAYER_VALID;

		//set remoteIp
		staticFilter.m_NetworkFilter.m_dwUnionSelector = IPV4;
		staticFilter.m_NetworkFilter.m_IPv4.m_ValidFields |= IP_V4_FILTER_DEST_ADDRESS;
		staticFilter.m_NetworkFilter.m_IPv4.m_DestAddress.m_AddressType=IP_RANGE_V4_TYPE;
		staticFilter.m_NetworkFilter.m_IPv4.m_DestAddress.m_IpRange.m_StartIp = remoteIp;
		staticFilter.m_NetworkFilter.m_IPv4.m_DestAddress.m_IpRange.m_EndIp = remoteIp;
	
		//set protocol
		if (protocolPort->Protocol!=0)
		{
			staticFilter.m_NetworkFilter.m_IPv4.m_ValidFields |= IP_V4_FILTER_PROTOCOL;
			staticFilter.m_NetworkFilter.m_IPv4.m_Protocol = protocolPort->Protocol;
		}

		//set port
		if (protocolPort->Protocol==IPPROTO_TCP || protocolPort->Protocol==IPPROTO_UDP)
		{
			staticFilter.m_ValidFields |= TRANSPORT_LAYER_VALID;
			staticFilter.m_TransportFilter.m_dwUnionSelector = TCPUDP;
			staticFilter.m_TransportFilter.m_TcpUdp.m_ValidFields |= TCPUDP_DEST_PORT;
			staticFilter.m_TransportFilter.m_TcpUdp.m_DestPort.m_StartRange = protocolPort->Port;
			staticFilter.m_TransportFilter.m_TcpUdp.m_DestPort.m_EndRange = protocolPort->Port;
		}

		filters->push_back(staticFilter);
	}
}

void BarbaPacketFilter::GetClientTunnelFilters(std::vector<STATIC_FILTER>* filters, u_long remoteIp, u_char protocol, std::vector<PortRange>* portRanges)
{
	for (size_t i=0; i<portRanges->size(); i++)
	{
		PortRange* portRange = &portRanges->at(i);

		STATIC_FILTER staticFilter = {0};
		staticFilter.m_Adapter = GetAdapterHandle();
		staticFilter.m_FilterAction = FILTER_PACKET_REDIRECT;
		staticFilter.m_dwDirectionFlags = PACKET_FLAG_ON_RECEIVE;
		staticFilter.m_ValidFields |= NETWORK_LAYER_VALID;

		//set remoteIp
		staticFilter.m_NetworkFilter.m_dwUnionSelector = IPV4;
		staticFilter.m_NetworkFilter.m_IPv4.m_ValidFields = IP_V4_FILTER_SRC_ADDRESS;
		staticFilter.m_NetworkFilter.m_IPv4.m_DestAddress.m_AddressType=IP_RANGE_V4_TYPE;
		staticFilter.m_NetworkFilter.m_IPv4.m_DestAddress.m_IpRange.m_StartIp = remoteIp;
		staticFilter.m_NetworkFilter.m_IPv4.m_DestAddress.m_IpRange.m_EndIp = remoteIp;

		//set protocol
		staticFilter.m_NetworkFilter.m_IPv4.m_ValidFields |= IP_V4_FILTER_PROTOCOL;
		staticFilter.m_NetworkFilter.m_IPv4.m_Protocol = protocol;

		//only TCP & UDP tunnel supported yet
		if (protocol==IPPROTO_TCP || protocol==IPPROTO_UDP)
		{
			staticFilter.m_ValidFields |= TRANSPORT_LAYER_VALID;
			staticFilter.m_TransportFilter.m_dwUnionSelector = TCPUDP;
			staticFilter.m_TransportFilter.m_TcpUdp.m_ValidFields |= TCPUDP_SRC_PORT;
			staticFilter.m_TransportFilter.m_TcpUdp.m_DestPort.m_StartRange = portRange->StartPort;
			staticFilter.m_TransportFilter.m_TcpUdp.m_DestPort.m_EndRange = portRange->EndPort;
		}

		filters->push_back(staticFilter);
	}
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

void BarbaPacketFilter::ApplyClientPacketFilter()
{
	std::vector<STATIC_FILTER> filters;
	for (size_t i=0; i<theClientApp->ConfigManager.Configs.size(); i++)
	{
		BarbaClientConfig* config = &theClientApp->ConfigManager.Configs[i];
		GetClientFilters(&filters, config);
	}
	
	if (!ApplyFilters(&filters))
		throw new BarbaException(_T("Could not set packet filtering!"));
}

void BarbaPacketFilter::ApplyServerPacketFilter()
{
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

bool BarbaPacketFilter::ApplyPacketFilter()
{
	try
	{
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
