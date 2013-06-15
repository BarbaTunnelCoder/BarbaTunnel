#include "stdafx.h"
#include "BarbaCourierUdpClient.h"
#include "BarbaUtils.h"


BarbaCourierUdpClient::BarbaCourierUdpClient(CreateStrcutUdp* cs)
	: BarbaCourierDatagram(cs)
{
	_SessionId = BarbaUtils::GetRandom(100000, UINT_MAX-1);
	LastKeepAliveTime = GetTickCount();
	LastKeepAliveSentChunkCount = 0;
	SentChunkCount = 0;
}


BarbaCourierUdpClient::~BarbaCourierUdpClient(void)
{
}

void BarbaCourierUdpClient::CheckKeepAlive()
{
	if (GetCreateStruct()->KeepAliveInterval==0)
		return; //ignore sending keep alive

	//check keep-alive time
	if ( BarbaUtils::GetTickDiff(LastKeepAliveTime) < GetCreateStruct()->KeepAliveInterval )
		return;
	LastKeepAliveTime = GetTickCount();
	
	//find number of ports that need to be alive
	int portsCount = (int)GetCreateStruct()->KeepAlivePortsCount - (int)(SentChunkCount-LastKeepAliveSentChunkCount);
	if (portsCount<=0)
		return;

	//ignore big keep alive
	if (portsCount>MaxKeepAlivePortsCount)
	{
		Log2(_T("Ignore sending 'keepAlive' for %d ports! You should decrease KeepAlivePortsCount."), portsCount);
		return;
	}

	//report
	Log2(_T("Sending 'keepAlive' for %d ports."), portsCount);

	//sending keepAlive
	for (size_t i=0; i<portsCount; i++)
	{
		std::string udpKeepAlive("@keepAlive");
		BarbaBuffer data((BYTE*)udpKeepAlive.data(), udpKeepAlive.size());
		SendData(&data);
	}


	LastKeepAliveSentChunkCount = SentChunkCount;
}

// GUID (32) | Data
bool BarbaCourierUdpClient::ProcessInboundPacket(PacketHelper* packet)
{
	if (!packet->IsUdp())
		return false;

	std::string tag = _T("BarbaTunnelUdp;");
	
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

	BarbaBuffer chunk(buffer.data() + offset, buffer.size()-offset);
	Log3(_T("Receiving UDP Chunk. ServerPort:%d, ClientPort:%d, Size: %d Bytes."), packet->GetSrcPort(), packet->GetDesPort(), chunk.size());
	SendChunkToInbound(&chunk);
	return true;
}

// GUID (32) | Session (4) | Data
void BarbaCourierUdpClient::SendChunkToOutbound(BarbaBuffer* chunk)
{
	//append UDP Tunnel signature
	// BarbaSign | SessionID | Chunk
	std::string tag = GetBarbaTag();

	BarbaBuffer buffer;
	buffer.reserve( tag.size() +  sizeof DWORD + chunk->size());
	buffer.append( (char*)tag.data(), tag.size() );
	buffer.append( &_SessionId, sizeof DWORD);
	buffer.append( chunk );
	Encrypt(buffer.data(), buffer.size(), 0);

	u_short clientPort = GetCreateStruct()->PortRange->GetRandomPort();
	u_short serverPort = GetCreateStruct()->PortRange->GetRandomPort();
	Log3(_T("Sending UDP Chunk. ServerPort:%d, ClientPort:%d, Size: %d bytes."), serverPort, clientPort, chunk->size());
	SendUdpPacketToOutbound(GetCreateStruct()->RemoteIp, clientPort, serverPort, &buffer);
	SentChunkCount++;
}

bool BarbaCourierUdpClient::PreReceiveDataControl(BarbaBuffer* data)
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
		SendInitCommand();
	return true;
}


void	 BarbaCourierUdpClient::SendInitCommand()
{
	std::tstring cmd = "BarbaCourierUdp;";
	BarbaUtils::SetKeyValue(&cmd, _T("command"), _T("udpInit"));
	BarbaUtils::SetKeyValue(&cmd, _T("keepAlivePortsCount"), GetCreateStruct()->KeepAlivePortsCount);
	BarbaUtils::SetKeyValue(&cmd, _T("maxChunkSize"), (int)GetCreateStruct()->MaxChunkSize);
	Log3(_T("Sending DataControl: %s"), cmd.data());

	BarbaBuffer data((BYTE*)cmd.data(), cmd.size());
	SendDataControl(&data);
}

void BarbaCourierUdpClient::Timer()
{
	BarbaCourierDatagram::Timer();
	CheckKeepAlive();
}
