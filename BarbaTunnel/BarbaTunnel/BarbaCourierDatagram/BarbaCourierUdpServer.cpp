#include "stdafx.h"
#include "BarbaCourierUdpServer.h"
#include "BarbaUtils.h"

BarbaCourierUdpServer::PortManager::PortManager()
{
	NextClientPortIndex = 0;
	PortPairs.reserve(20); //beaware of NAT timeout
}

void BarbaCourierUdpServer::PortManager::AddPort(u_short serverPort, u_short clientPort)
{
	PortPair pair;
	pair.ServerPort = serverPort;
	pair.ClientPort = clientPort;

	if (PortPairs.size()<PortPairs.capacity())
		PortPairs.append(pair);
	else 
		PortPairs[NextClientPortIndex] = pair;

	NextClientPortIndex++;
	if (NextClientPortIndex>=PortPairs.capacity())
		NextClientPortIndex = 0;
}

void BarbaCourierUdpServer::PortManager::FindPort(u_short* serverPort, u_short* clientPort)
{
	if (PortPairs.empty())
	{
		*serverPort = 0;
		*clientPort = 0;
		return;
	}
	
	u_int index = BarbaUtils::GetRandom(0, (int)PortPairs.size()-1);
	PortPair pair = PortPairs[index];
	*serverPort = pair.ServerPort;
	*clientPort = pair.ClientPort;
}

BarbaCourierUdpServer::BarbaCourierUdpServer(CreateStrcutUdp* cs)
	: BarbaCourierDatagram(cs)
{
}


BarbaCourierUdpServer::~BarbaCourierUdpServer(void)
{
}


// GUID (32) | Session (4) | Data
bool BarbaCourierUdpServer::ProcessInboundPacket(PacketHelper* packet)
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

	DWORD sessionId = *(DWORD*)(buffer.data() + offset);
	offset += sizeof sessionId;
	
	//create new session if session is 0
	if (_SessionId==0)
		_SessionId = sessionId;
	else if (_SessionId != sessionId)
		return false;

	portManager.AddPort(packet->GetDesPort(), packet->GetSrcPort());
	BarbaBuffer chunk(buffer.data() + offset, buffer.size()-offset);
	Log3(_T("Receiving UDP Chunk. ClientPort:%d, ServerPort:%d, Size: %d Bytes."), packet->GetDesPort(), packet->GetSrcPort(), chunk.size());
	SendChunkToInbound(&chunk);
	return true;
}

// GUID (32) | Data
void BarbaCourierUdpServer::SendChunkToOutbound(BarbaBuffer* chunk)
{
	//append UDP Tunnel signaturel
	// BarbaSign | Chunk
	BarbaBuffer buffer;
	buffer.reserve( sizeof GUID +  chunk->size() );
	buffer.append( GetBarbaSign(), sizeof GUID );
	buffer.append( chunk );
	Encrypt(buffer.data(), buffer.size(), 0);
	
	u_short serverPort;
	u_short clientPort;
	portManager.FindPort(&serverPort, &clientPort);
	Log3(_T("Sending UDP Chunk. SessionId: %x, ClientPort:%d, ServerPort:%d, Size: %d Bytes."), _SessionId, clientPort, serverPort, chunk->size());
	SendUdpPacketToOutbound(GetCreateStruct()->RemoteIp, serverPort, clientPort, &buffer);
}

