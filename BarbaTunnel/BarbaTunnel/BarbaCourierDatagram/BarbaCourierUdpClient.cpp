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
	LastInitCommandSentTime = 0;
	IsInitCommandAckRecieved = false;
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
		BarbaBuffer keepAlive;
		SendControlCommand(_T("keepAlive"), false);
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
	CheckInitCommand();
	CheckKeepAlive();

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
	Log3(_T("Sending UDP Chunk. ServerPort:%d, ClientPort:%d, Size: %d Bytes."), serverPort, clientPort, chunk->size());
	SendUdpPacketToOutbound(GetCreateStruct()->RemoteIp, clientPort, serverPort, &buffer);
	SentChunkCount++;
}

void BarbaCourierUdpClient::ProcessControlCommand(std::tstring command)
{
	BarbaCourierDatagram::ProcessControlCommand(command); //reprot it 

	//initialize server
	std::tstring cmd = BarbaUtils::GetKeyValueFromString(command.data(), _T("command"));
	if (cmd.compare(_T("initAck"))==0)
		IsInitCommandAckRecieved = true;
}


void	 BarbaCourierUdpClient::CheckInitCommand()
{
	if (IsInitCommandAckRecieved || (GetTickCount()-LastInitCommandSentTime)<3000)
		return;

	LastInitCommandSentTime = GetTickCount();

	std::tstring cmd;
	BarbaUtils::SetKeyValue(&cmd, _T("command"), _T("init"));
	BarbaUtils::SetKeyValue(&cmd, _T("keepAlivePortsCount"), GetCreateStruct()->KeepAlivePortsCount);
	BarbaUtils::SetKeyValue(&cmd, _T("maxChunkSize"), (int)GetCreateStruct()->MaxChunkSize);
	SendControlCommand(cmd);
}
