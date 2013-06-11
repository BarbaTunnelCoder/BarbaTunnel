#include "stdafx.h"
#include "BarbaCourierUdpClient.h"
#include "BarbaUtils.h"


BarbaCourierUdpClient::BarbaCourierUdpClient(CreateStrcutUdp* cs)
	: BarbaCourierDatagram(cs)
{
	_SessionId = BarbaUtils::GetRandom(100000, UINT_MAX-1);
}


BarbaCourierUdpClient::~BarbaCourierUdpClient(void)
{
}


// GUID (32) | Data
bool BarbaCourierUdpClient::ProcessInboundPacket(PacketHelper* packet)
{
	if (!packet->IsUdp())
		return false;

	//get udp data
	BarbaBuffer buffer(packet->GetUdpPayload(), packet->GetUdpPayloadLen());
	if (buffer.size()< (sizeof GUID + sizeof DWORD) )
	{
		Log3(_T("Invalid UDP size for a UDP chunk! Invalid key or overlapped port!"));
		return false;
	}

	//decrypt packet
	Decrypt(buffer.data(), buffer.size(), 0);

	//extract data
	int offset = 0;
	GUID sign = *(GUID*)(buffer.data() + offset);
	offset += sizeof sign;
	if (sign!=*GetBarbaSign())
	{
		Log3(_T("Could not find Barba Signature in chunk! Invalid key or overlapped port!"));
		return false; //not barba packet
	}

	BarbaBuffer chunk(buffer.data() + offset, buffer.size()-offset);
	Log3(_T("Receiving UDP Chunk. ClientPort:%d, ServerPort:%d, Size: %d Bytes."), packet->GetDesPort(), packet->GetSrcPort(), chunk.size());
	SendChunkToInbound(&chunk);
	return true;
}

// GUID (32) | Session (4) | Data
void BarbaCourierUdpClient::SendChunkToOutbound(BarbaBuffer* chunk)
{
	//append UDP Tunnel signature
	// BarbaSign | SessionID | Chunk
	BarbaBuffer buffer;
	buffer.reserve( sizeof GUID +  sizeof DWORD + chunk->size());
	buffer.append( GetBarbaSign(), sizeof GUID);
	buffer.append( &_SessionId, sizeof DWORD);
	buffer.append( chunk );
	Encrypt(buffer.data(), buffer.size(), 0);

	u_short clientPort = GetCreateStruct()->PortRange->GetRandomPort();
	u_short serverPort = GetCreateStruct()->PortRange->GetRandomPort();
	Log3(_T("Sending UDP Chunk. ClientPort:%d, ServerPort:%d, Size: %d Bytes."), clientPort, serverPort, chunk->size());
	SendUdpPacketToOutbound(GetCreateStruct()->RemoteIp, clientPort, serverPort, &buffer);
}
