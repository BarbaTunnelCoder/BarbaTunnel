#include "stdafx.h"
#include "BarbaCourierUdpServer.h"
#include "BarbaUtils.h"

BarbaCourierUdpServer::PortManager::PortManager()
{
	NextPortIndex = 0;
	SetMaxPorts(1);
}

void BarbaCourierUdpServer::PortManager::SetMaxPorts(u_short value)
{
	value = min(0xFFFF, value); //i know max short can have maximum 0xFFFF, but maybe i change type later
	value = max(1, value); //couldn't be less than 1
	MaxPortsCount = value;
	PortPairs.reserve(value);
	if (PortPairs.size()>value)
		PortPairs.resize(value);
	if (NextPortIndex>=value)
		NextPortIndex = 0;
}

u_short BarbaCourierUdpServer::PortManager::GetMaxPorts()
{
	return MaxPortsCount;
}

void BarbaCourierUdpServer::PortManager::AddPort(u_short serverPort, u_short clientPort)
{
	PortPair pair;
	pair.ServerPort = serverPort;
	pair.ClientPort = clientPort;

	if (PortPairs.size()<MaxPortsCount)
		PortPairs.append(pair);
	else 
		PortPairs[NextPortIndex] = pair;

	NextPortIndex++;
	if (NextPortIndex>=MaxPortsCount)
		NextPortIndex = 0;
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
	portManager.	SetMaxPorts(cs->KeepAlivePortsCount);
}


BarbaCourierUdpServer::~BarbaCourierUdpServer(void)
{
}

void BarbaCourierUdpServer::SendInitRequest()
{
	std::tstring cmd = "BarbaCourierUdp;sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss;";
	BarbaUtils::SetKeyValue(&cmd, _T("command"), _T("udpInit"));
	Log3(_T("Sending DataControl: %s"), cmd.data());

	BarbaBuffer data((char*)cmd.data(), cmd.size());
	SendDataControl(&data);
}


// GUID (32) | Session (4) | Data
bool BarbaCourierUdpServer::ProcessInboundPacket(PacketHelper* packet)
{
	if (!packet->IsUdp())
		return false;

	std::string tag = GetBarbaTag();
	
	//get udp data
	BarbaBuffer buffer(packet->GetUdpPayload(), packet->GetUdpPayloadLen());
	if (buffer.size()< (tag.size() + sizeof DWORD) )
	{
		Log3(_T("Invalid UDP size for a UDP chunk! Invalid key or overlapped port!"));
		return false;
	}

	//decrypt packet
	Decrypt(buffer.data(), buffer.size(), 0);

	//extract data
	int offset = 0;
	std::string tag2((char*)buffer.data() + offset, tag.size());
	offset += (int)tag.size();
	if (tag2.compare(tag)!=0)
	{
		Log3(_T("Could not find Barba Signature in chunk! Invalid key or overlapped port!"));
		return false; //not barba packet
	}

	DWORD sessionId = *(DWORD*)(buffer.data() + offset);
	offset += sizeof sessionId;
	
	//create new session if session is 0
	bool isNewSession = _SessionId==0;
	if (isNewSession)
	{
		_SessionId = sessionId;
	}
	else if (_SessionId != sessionId)
	{
		return false;
	}

	portManager.AddPort(packet->GetDesPort(), packet->GetSrcPort());
	BarbaBuffer chunk(buffer.data() + offset, buffer.size()-offset);
	Log3(_T("Receiving UDP Chunk. ServerPort:%d, ClientPort:%d, Size: %d bytes."), packet->GetDesPort(), packet->GetSrcPort(), chunk.size());
	SendChunkToInbound(&chunk);

	//Request Init data
	if (isNewSession)
		SendInitRequest();
	return true;
}

// GUID (32) | Data
void BarbaCourierUdpServer::SendChunkToOutbound(BarbaBuffer* chunk)
{
	//append UDP Tunnel signaturel
	std::string tag = "BarbaTunnelUdp;";

	// BarbaSign | Chunk
	BarbaBuffer buffer;
	buffer.reserve( tag.size() +  chunk->size() );
	buffer.append( (char*)tag.data(), tag.size() );
	buffer.append( chunk );
	Encrypt(buffer.data(), buffer.size(), 0);
	
	u_short serverPort;
	u_short clientPort;
	portManager.FindPort(&serverPort, &clientPort);
	if (clientPort==0 || serverPort==0)
		return;

	Log3(_T("Sending UDP Chunk. SessionId: %x, ServerPort:%d, ClientPort:%d, Size: %d bytes."), _SessionId, serverPort, clientPort, chunk->size());
	SendUdpPacketToOutbound(GetCreateStruct()->RemoteIp, serverPort, clientPort, &buffer);
}

bool BarbaCourierUdpServer::PreReceiveData(BarbaBuffer* data)
{
	std::string udpKeepAlive("@keepAlive");
	if (data->size()==udpKeepAlive.size() && memcmp(data->data(), udpKeepAlive.data(), udpKeepAlive.size()))
		return true;
	return false;
}

bool BarbaCourierUdpServer::PreReceiveDataControl(BarbaBuffer* data)
{
	if (BarbaCourierDatagram::PreReceiveDataControl(data))
		return true;

	//check is DataContorl belong to me
	std::string tag = "BarbaCourierUdp;";
	bool isMine = data->size()>=tag.size() && memcmp(data->data(), tag.data(), tag.size())==0;
	if (!isMine)
		return false;

	//process
	std::string command((char*)data->data(), data->size());
	std::tstring cmdName = BarbaUtils::GetKeyValueFromString(command.data(), _T("command"));
	Log3(_T("Receiving DataControl: %s"), command.data());

	//initialize server
	if (cmdName.compare(_T("udpInit"))==0)
	{
		int keepAlivePortsCount = BarbaUtils::GetKeyValueFromString(command.data(), _T("keepAlivePortsCount"), 0);
		if (keepAlivePortsCount!=0) portManager.SetMaxPorts((u_short)keepAlivePortsCount); 

		int maxChunkSize = BarbaUtils::GetKeyValueFromString(command.data(), _T("maxChunkSize"), 0);
		if (maxChunkSize!=0) GetCreateStruct()->MaxChunkSize = maxChunkSize;
	}

	return true;
}
