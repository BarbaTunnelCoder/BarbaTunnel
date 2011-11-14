#include "StdAfx.h"
#include "BarbaApp.h"
extern CNdisApi			api;


void Crypt(BYTE* buffer, int len)
{
	for (int i=0; i<len; i++)
	{
		buffer[i] ^= i+1;
	}
}

void CryptUdp(ether_header_ptr ethHeader)
{
	iphdr_ptr ipHeader = (iphdr*)(ethHeader + 1);
	udphdr_ptr udpHeader = (udphdr_ptr)(((PUCHAR)ipHeader) + ipHeader->ip_hl*4);

	int ipLen = ntohs( ipHeader->ip_len );
	int udpPayloadLen = ntohs(udpHeader->length) - sizeof(udphdr);
	BYTE* udpPayload = (BYTE*)udpHeader + sizeof(udphdr);
	Crypt(udpPayload, udpPayloadLen);
	BarbaUtils::RecalculateIPChecksum(ipHeader);
}

void CryptTcp(ether_header_ptr ethHeader)
{
	iphdr_ptr ipHeader = (iphdr*)(ethHeader + 1);
	tcphdr_ptr tcpHeader = (tcphdr_ptr)(((PUCHAR)ipHeader) + ipHeader->ip_hl*4);

	int addIpHeaderLen = ipHeader->ip_hl*4 - sizeof (iphdr);
	BYTE* addIpHeader = (BYTE*)(ipHeader + 1);

	int addTcpHeaderLen = tcpHeader->th_off*4 - sizeof (tcphdr);
	BYTE* addTcpHeader = (BYTE*)(tcpHeader + 1);

	int ipLen = ntohs( ipHeader->ip_len );
	int tcpPayloadLen = ipLen - ipHeader->ip_hl*4 - tcpHeader->th_off*4;
	BYTE* tcpPayload = (BYTE*)tcpHeader + tcpHeader->th_off*4;

	Crypt(addIpHeader, addIpHeaderLen);
	Crypt(addTcpHeader, addTcpHeaderLen);
	Crypt(tcpPayload, tcpPayloadLen);

	BarbaUtils::RecalculateIPChecksum(ipHeader);
}


void printPacketInfo(ether_header_ptr ethHeader)
{
	PacketHelper packet(ethHeader);

	if (ntohs(ethHeader->h_proto)!=ETH_P_IP)
	{
		printf("Packet is not IP paket!\n");
		return;
	}

	//printf("", packet.ipHeader->ip_sum);



	//int ipLen = ntohs( ipHeader->ip_len );
	//int tcpPayloadLen = ipLen - ipHeader->ip_hl*4 - tcpHeader->th_off*4;
	//printf("sport: %d, dport:%d, tcpFlag:%x, ipLen:%d, ipHL: %d, tcpOff: %d, tcpPayloadLen:%d, ipCheckSum:%d\n", ntohs(tcpHeader->th_sport), ntohs(tcpHeader->th_dport), tcpHeader->th_off, 
		//ipLen, ipHeader->ip_hl, tcpHeader->th_off, tcpPayloadLen, BarbaUtils::CheckSum((USHORT*)ipHeader, ipLen));

}


bool fakedInit = false;
void BarbaConnection::InitFakeConnection(ether_header_ptr ethHeader)
{
	//iphdr_ptr ipHeader = (iphdr*)(ethHeader + 1);
	//tcphdr_ptr tcpHeader = (tcphdr_ptr)(((PUCHAR)ipHeader) + ipHeader->ip_hl*4);

	fakedInit = true;
}

ether_header_ptr BarbaConnection::CreateBarbaPacket2(ether_header_ptr ethHeader)
{
	//if (!theApp->IsServer() && !fakedInit)
		//InitFakeConnection(ethHeader);

	printf("\nCreating BarbaPacket, orgPacket:\n");
	printPacketInfo(ethHeader);

	iphdr_ptr ipHeader = (iphdr*)(ethHeader + 1);
	tcphdr_ptr tcpHeader = (tcphdr_ptr)(((PUCHAR)ipHeader) + ipHeader->ip_hl*4);


	//create BarbaHeader [Signature + Version]
	BarbaHeader barbaHeader;
	memcpy_s(&barbaHeader.Signature, sizeof barbaHeader.Signature, theApp->GetBarbaSign(), sizeof barbaHeader.Signature);
	barbaHeader.Version = 1;
	
	//create Barba data [ Barba Info + Original IP Packet]
	size_t ipLen = (u_short)ntohs(ipHeader->ip_len);
	size_t barbaDataSize = sizeof BarbaHeader + ipLen;
	BYTE barbaData[MAX_ETHER_FRAME];
	memcpy_s(barbaData, barbaDataSize, &barbaHeader, sizeof BarbaHeader);
	memcpy_s((BYTE*)barbaData+sizeof BarbaHeader, barbaDataSize-sizeof BarbaHeader, ipHeader, ipLen);
	if ( !fakedInit)
	{
		barbaDataSize = 0;
		fakedInit= true;

	}


	//barbaDataSize = 0;
	
	//create Barba Tcp Segment [TCP header + Barba data]
	size_t barbaTcpSegmentSize = sizeof tcphdr + barbaDataSize;
	BYTE barbaTcpSegmentBytes[MAX_ETHER_FRAME];
	tcphdr_ptr barbaTcpHeader = (tcphdr_ptr)barbaTcpSegmentBytes;
	memset(barbaTcpHeader, 0, sizeof tcphdr); //reset header
	memcpy(barbaTcpHeader, tcpHeader, sizeof tcphdr);
	if (theApp->IsServer())
		barbaTcpHeader->th_sport = htons( Config->TunnelProtocol.Port );
	else
		barbaTcpHeader->th_dport = htons( Config->TunnelProtocol.Port );
	u_short b = ntohs(barbaTcpHeader->th_dport);


	barbaTcpHeader->th_off = 5; //standard tcp header length
	barbaTcpHeader->th_win = 0;
	barbaTcpHeader->th_urp = 0;
	barbaTcpHeader->th_x2 = 0;
	memcpy_s((BYTE*)barbaTcpHeader + sizeof tcphdr, barbaTcpSegmentSize-sizeof tcphdr, barbaData, barbaDataSize);

	//Create Barba IP header
	iphdr barbaIpHeader = *ipHeader;
	barbaIpHeader.ip_len = htons( sizeof iphdr + barbaTcpSegmentSize );
	barbaIpHeader.ip_hl = 5; //standard ip header length
	barbaIpHeader.ip_p = IPPROTO_TCP;
	barbaIpHeader.ip_sum = 0;

	//Create Barba Packet [Ethernet + Barba IP Header + Barba TCP Segment]
	size_t barbaPacketSize = sizeof ether_header + sizeof iphdr + barbaTcpSegmentSize;
	ether_header_ptr barbaPacket = (ether_header_ptr)new BYTE[barbaPacketSize];
	memcpy_s(barbaPacket, sizeof ether_header, ethHeader, sizeof ether_header);
	memcpy_s((BYTE*)barbaPacket + sizeof ether_header, barbaPacketSize - sizeof ether_header, &barbaIpHeader, sizeof iphdr);
	memcpy_s((BYTE*)barbaPacket + sizeof ether_header + sizeof iphdr, barbaPacketSize - sizeof ether_header - sizeof iphdr, barbaTcpHeader, barbaTcpSegmentSize);

	//recalculate checksum
	iphdr_ptr newIpHeader = (iphdr*)(barbaPacket + 1);
	tcphdr_ptr newTcpHeader = (tcphdr_ptr)(((PUCHAR)newIpHeader) + newIpHeader->ip_hl*4);
	BarbaUtils::RecalculateTCPChecksum(newIpHeader);
	BarbaUtils::RecalculateIPChecksum(newIpHeader);

	printf("\nCreating BarbaPacket, BarbaPacket:\n\n");
	printPacketInfo(barbaPacket);

	return barbaPacket;
}


ether_header_ptr BarbaConnection::ExtractOrigianlPacketFromBarbaPacket(ether_header_ptr ethHeader)
{
	printf("\nExtracting BarbaPacket, BarbaPacket:\n");
	printPacketInfo(ethHeader);

	//ipHeader
	iphdr_ptr ipHeader = (iphdr*)(ethHeader + 1);
	if (ipHeader->ip_p!=IPPROTO_TCP)
		return NULL; //

	//tcpHeader
	tcphdr_ptr tcpHeader = (tcphdr_ptr)(((PUCHAR)ipHeader) + ipHeader->ip_hl*4);
	BYTE* tcpSegmentData = (BYTE*)tcpHeader + (tcpHeader->th_off*4);
	int tcpSegmentDataSize = (u_short)ntohs(ipHeader->ip_len) - ipHeader->ip_hl*4 - tcpHeader->th_off*4;

	//check is Barba Packet
	if (tcpSegmentDataSize<sizeof BarbaHeader)
		return NULL;
	BarbaHeader* barbaHeader = (BarbaHeader*)tcpSegmentData;
	if (memcmp(&barbaHeader->Signature, theApp->GetBarbaSign(), sizeof barbaHeader->Signature)!=0)
		return NULL;

	//create original ip-packet
	size_t orgIpPacketSize = tcpSegmentDataSize - sizeof BarbaHeader;
	iphdr_ptr orgIpPacket = (iphdr_ptr)((BYTE*)tcpSegmentData + sizeof BarbaHeader);
	size_t newPacketSize = orgIpPacketSize + sizeof ether_header;
	ether_header_ptr newPacket = (ether_header_ptr)new BYTE[newPacketSize];
	memcpy_s(newPacket, sizeof ether_header, ethHeader, sizeof ether_header);
	memcpy_s((BYTE*)newPacket + sizeof ether_header, newPacketSize-sizeof ether_header, orgIpPacket, orgIpPacketSize);

	printf("\nExtracting BarbaPacket, OrgPacket:\n");
	printPacketInfo(newPacket);

	//correct for original packet for NAT
	iphdr_ptr newIpHeader = (iphdr*)(newPacket + 1);
	tcphdr_ptr newTcpHeader =  (tcphdr_ptr)(((PUCHAR)newIpHeader) + newIpHeader->ip_hl*4);
	newIpHeader->ip_src = ipHeader->ip_src;
	newIpHeader->ip_ttl = ipHeader->ip_ttl;
	if (theApp->IsServer())
		newTcpHeader->th_sport = tcpHeader->th_sport;
	else
		newTcpHeader->th_dport = tcpHeader->th_dport;
	BarbaUtils::RecalculateTCPChecksum(newIpHeader);
	BarbaUtils::RecalculateIPChecksum(newIpHeader);

	printf("Extracting BarbaPacket, OrgPacket-NAT:\n\n");
	printPacketInfo(newPacket);

	return newPacket;
}


ether_header_ptr BarbaConnection::CreateBarbaPacket(INTERMEDIATE_BUFFER* packet)
{
	ether_header_ptr ethHeader = (ether_header*)packet->m_IBuffer;
	ethHeader = CreateBarbaPacket2(ethHeader);
	iphdr_ptr ipHeader = (iphdr*)(ethHeader + 1);
	packet->m_Length = sizeof(ether_header) + htons(ipHeader->ip_len);
	memcpy(packet->m_IBuffer, ethHeader, packet->m_Length);
	return 0;
}

void SendPing(INTERMEDIATE_BUFFER* packet)
{
	printf("Sending Ping..\n");

	ether_header_ptr ethHeader = (ether_header*)packet->m_IBuffer;
	iphdr_ptr ipHeader = (iphdr*)(ethHeader + 1);
	int orgIpLen = ntohs(ipHeader->ip_len);

	//my ping
	BYTE buffer[MAX_ETHER_FRAME] = {0};
	memset(buffer, 7, MAX_ETHER_FRAME);
	ether_header_ptr eth = (ether_header_ptr)buffer;
	iphdr_ptr ip = (iphdr_ptr)(buffer + sizeof ether_header);

	memcpy(eth->h_source, ethHeader->h_source, ETH_ALEN);
	memcpy(eth->h_dest, ethHeader->h_dest, ETH_ALEN);
	eth->h_proto = ethHeader->h_proto;
	//memcpy(eth, ethHeader, sizeof ether_header);


	ip->ip_src.S_un.S_addr = BarbaUtils::ConvertStringIp(_T("192.168.0.21"));
	ip->ip_dst.S_un.S_addr = BarbaUtils::ConvertStringIp(_T("76.72.163.224"));
	ip->ip_p = IPPROTO_ICMP;
	ip->ip_v = 4;
	ip->ip_hl = 5;
	ip->ip_id = 0;
	ip->ip_tos = 0;
	ip->ip_off = 0;
	ip->ip_ttl = 128;
	ip->ip_len = htons( ntohs(15360) + 0);
	int dataLen = orgIpLen-ipHeader->ip_hl*4;
	memcpy((BYTE*)ip + ip->ip_hl*4, (BYTE*)ipHeader + ipHeader->ip_hl*4, orgIpLen-ipHeader->ip_hl*4);

	BarbaUtils::RecalculateIPChecksum(ip);

	int len = sizeof ether_header + ntohs(ip->ip_len);
	memcpy(packet->m_IBuffer, eth, MAX_ETHER_FRAME);
	packet->m_Length = len;
}

bool BarbaConnection::ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer)
{
	PacketHelper orgPacket(orgPacketBuffer, 0);
	orgPacket.SetEthHeader(barbaPacket->ethHeader);
	orgPacket.SetIpPacket((iphdr_ptr)barbaPacket->GetUdpPayload());
	return true;
}

size_t BarbaConnection::CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer)
{
	PacketHelper barbaPacket(barbaPacketBuffer, IPPROTO_UDP);
	barbaPacket.SetEthHeader(packet->ethHeader);
	barbaPacket.ipHeader->ip_ttl = packet->ipHeader->ip_ttl;
	barbaPacket.ipHeader->ip_v = packet->ipHeader->ip_v;
	barbaPacket.ipHeader->ip_id = 56;
	barbaPacket.ipHeader->ip_off = 0;
	barbaPacket.SetSrcIp(packet->GetSrcIp());
	barbaPacket.SetDesIp(this->PeerIP);
	barbaPacket.SetSrcPort(this->Config->TunnelProtocol.Port);
	barbaPacket.SetDesPort( theApp->IsServer() ? this->PeerPort : this->Config->TunnelProtocol.Port);
	barbaPacket.SetUdpPayload((BYTE*)packet->ipHeader, packet->GetIpLen());
	if (theApp->IsServer()) barbaPacket.SetDesEthAddress(this->PeerEthAddress);
	return barbaPacket.GetPacketLen();
}

//This function called when the connection should process the current IP
//In receive mode it should check signature after decrypt
//@return true if it process the packet
bool BarbaConnection::ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags==PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer->m_IBuffer);

	if (theApp->IsServer())
	{
		if (send)
		{
			//test
			//packet.SetDesIp(this->PeerLocalIp);
			//packet.RecalculateChecksum();
			//packet.SetDesEthAddress(this->PeerEthAddress);
			//return true;
			

			packet.SetDesIp(this->PeerLocalIp);

			//static int i =0;
			//printf("\n\nServer sending packet!: ");
			//printf("\n%d, sum: %d\n", i++, packet.ipHeader->ip_sum);
			//printf("\norgPacket len: %d, sport%d, dport:%d\n", packet.GetPacketLen(), packet.GetSrcPort(), packet.GetDesPort());
			//BarbaUtils::PrintIp(packet.GetDesIp());
			//printf("\n");


			//Create Barba packet
			BYTE barbaPacketBuffer[MAX_ETHER_FRAME];
			size_t length = CreateUdpBarbaPacket(&packet, barbaPacketBuffer);
			PacketHelper barbaPacket(barbaPacketBuffer);

			packet.SetEthPacket(barbaPacket.ethHeader);
			CryptUdp(packet.ethHeader);
			packet.RecalculateChecksum();
			packetBuffer->m_Length = packet.GetPacketLen();
			return true;
		}
		else
		{
			//extract Barba packet
			BYTE orgPacketBuffer[MAX_ETHER_FRAME];
			//printf("barba tunnel\n");
			CryptUdp(packet.ethHeader);
			if (!ExtractUdpBarbaPacket(&packet, orgPacketBuffer))
				return false;
			PacketHelper orgPacket(orgPacketBuffer);
			
			//prepare for NAT
			if (this->PeerFakeIp==0)
			{
				this->PeerFakeIp = BarbaUtils::ConvertStringIp("192.168.17.1");// + theApp->IpInc++;
				this->PeerLocalIp = orgPacket.GetSrcIp();
				memcpy(this->PeerEthAddress, packet.ethHeader->h_source, ETH_ALEN);
			}
			orgPacket.SetSrcIp(this->PeerFakeIp);
			orgPacket.RecalculateChecksum();

			//printf("barba restored\n");

			//replace current packet with barba packet
			//static int i = 0;
			packet.SetEthPacket(orgPacket.ethHeader);
			packet.RecalculateChecksum();
			packetBuffer->m_Length = packet.GetPacketLen();
			return true;
		}
	}
	else
	{
		if (send)
		{
			//static int i = 0;
			//printf("\norgPacket len: %d, sport%d, dport:%d", packet.GetPacketLen(), packet.GetSrcPort(), packet.GetDesPort());
			//if (packet.GetDesPort()!=1723)
				//return false;
			
			//Create Barba packet
			BYTE barbaPacket[MAX_ETHER_FRAME];
			size_t length = CreateUdpBarbaPacket(&packet, barbaPacket);
			
			//check result
			if (length==0)
				return false;

			//replace current packet with barba packet
			packet.SetEthPacket((ether_header_ptr)barbaPacket);
			CryptUdp(packet.ethHeader);
			packet.RecalculateChecksum();
			packetBuffer->m_Length = packet.GetPacketLen();

			return true;
		}
		else
		{
			//extract Barba packet
			BYTE orgPacketBuffer[MAX_ETHER_FRAME];
			CryptUdp(packet.ethHeader);
			if (!ExtractUdpBarbaPacket(&packet, orgPacketBuffer))
				return false;
			PacketHelper orgPacket(orgPacketBuffer);
	
			packet.SetEthPacket(orgPacket.ethHeader);
			packet.RecalculateChecksum();
			packetBuffer->m_Length = packet.GetPacketLen();

			static int i =0;
			//printf("\n%d, sum: %d", i++, packet.ipHeader->ip_sum);
			//printf("\norgPacket len: %d, sport%d, dport:%d\n", packet.GetPacketLen(), packet.GetSrcPort(), packet.GetDesPort());
			//BarbaUtils::PrintIp(orgPacket.GetSrcIp());
			//printf(" -- ");
			//BarbaUtils::PrintIp(orgPacket.GetDesIp());

			//extract org packet from barba packet
			return true;
		}

	}

	return false;
}
