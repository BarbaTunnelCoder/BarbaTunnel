#include "stdafx.h"
#include "BarbaCourierDatagram.h"
#include "BarbaException.h"
#include "BarbaUtils.h"

//Message Implementation
BarbaCourierDatagram::Message::Message(DWORD id, DWORD totalChunks)
{
	AddedChunksCount = 0;
	Id = id;
	Chunks.assign(totalChunks);
	LastUpdateTime = GetTickCount(); 
}

BarbaCourierDatagram::Message::Message(DWORD id, BarbaBuffer* data, DWORD maxChunkSize)
{
	Construct(id, data->data(), data->size(), maxChunkSize);
}

BarbaCourierDatagram::Message::Message(DWORD id, BYTE* data, size_t dataSize, DWORD maxChunkSize)
{
	Construct(id, data, dataSize, maxChunkSize);
}

void BarbaCourierDatagram::Message::Construct(DWORD id, BYTE* data, size_t dataSize, DWORD maxChunkSize)
{
	AddedChunksCount = 0;
	Id = id;
	maxChunkSize = max(maxChunkSize, 100); //couldnt be less than 100
	DWORD totalChunks = (DWORD) (dataSize / maxChunkSize);
	if ( (dataSize % maxChunkSize)!=0 ) totalChunks++;

	Chunks.assign( totalChunks );
	for (int i=0; i<(int)totalChunks; i++)
		AddChunk(i, data + (i*maxChunkSize), min(dataSize-(i*maxChunkSize), maxChunkSize));

	LastUpdateTime = GetTickCount(); 
}

void BarbaCourierDatagram::Message::AddChunk(DWORD chunkIndex, BYTE* data, size_t dataSize)
{
	if (chunkIndex>=Chunks.size())
		throw new BarbaException(_T("ChunkIndex is out of range!"));
	if ( Chunks[chunkIndex]==NULL ) AddedChunksCount++;
	Chunks[chunkIndex] = new BarbaBuffer(data, dataSize);
	LastUpdateTime = GetTickCount(); 
}

bool BarbaCourierDatagram::Message::IsCompleted()
{
	return AddedChunksCount==Chunks.size();
}

void BarbaCourierDatagram::Message::GetData(BarbaBuffer* data)
{
	//calculate data size for reservation
	size_t totalSize = 0;
	for (size_t i=0; i<Chunks.size(); i++)
		totalSize += Chunks[i]->size();
	data->reserve(totalSize);

	//append data
	for (size_t i=0; i<Chunks.size(); i++)
		data->append(Chunks[i]);
}

BarbaCourierDatagram::Message::~Message()
{
	for (size_t i=0; i<Chunks.size(); i++)
		delete Chunks[i];
}

// *************** DataControlManager Implementation
BarbaCourierDatagram::DataControlManager::DataControlManager()
{
	LastSentId = 0;
	LastReceivedId = 0;
	LastSentTime = 0;
	NextId = 0;
}

BarbaCourierDatagram::DataControlManager::~DataControlManager()
{
	while(!Datas.empty())
		delete Datas.removeTail();
}

// BarbaTunnelComm | id | type | data
bool BarbaCourierDatagram::DataControlManager::CheckDataControl(BarbaBuffer* data)
{
	std::string tag = "BarbaTunnelComm;";
	bool ret = data->size()>=(tag.size() + sizeof DWORD + sizeof BYTE) && memcmp(data->data(), tag.data(), tag.size())==0;
	if (!ret)
		return false;

	int offset = (int)tag.size();
	DWORD id = *(DWORD*)(data->data() + offset);
	offset += sizeof DWORD;

	BYTE type = *(BYTE*)(data->data() + offset);
	offset += sizeof BYTE;

	BarbaBuffer buf;
	if (type==0)
	{
		//only accept next id
		if (id<=LastReceivedId && id!=0)
		{
			Courier->Log3(_T("Dropping DataControl. Already Received! %d bytes, Id: %d."), buf.size(), id); 
			SendAck(id);
		}
		else
		{
			LastReceivedId = id;
			buf.assign(data->data() + offset, data->size()-offset);
			Courier->Log3(_T("Receiving DataControl. %d bytes, Id=%d."), buf.size(), id); 
			if (Courier->PreReceiveDataControl(&buf))
				Courier->ReceiveDataControl(&buf);
			SendAck(id);
		}
	}
	else if (type==1)
	{
		Courier->Log3(_T("Receiving DataControl Ack for id: id."), buf.size());
		if (id==LastSentId)
		{
			if (!Datas.empty())
				delete Datas.removeHead();
			LastSentTime = 0;
			NextId++;
			Process();
		}
	}
	else
	{
		Courier->Log3(_T("Unknown DataControl Type!"));
	}

	return true;
}

void BarbaCourierDatagram::DataControlManager::SendAck(DWORD id)
{
	std::string tag = "BarbaTunnelComm;";
	BarbaBuffer buf;
	buf.append((BYTE*)tag.data(), tag.size());
	buf.append((BYTE*)&id, sizeof DWORD);
	buf.append(1); //ack
	Courier->Log3(_T("Sending DataControl Ack for id: %d."), id); 
	Courier->SendData(&buf);
}

void BarbaCourierDatagram::DataControlManager::Send(BarbaBuffer* data)
{
	BarbaBuffer* buffer = new BarbaBuffer(data);
	Datas.addTail(buffer);
	Process();
}

void BarbaCourierDatagram::DataControlManager::Process()
{
	if (BarbaUtils::GetTickDiff(LastSentTime)<4000)
		return;
	LastSentTime = GetTickCount();

	if (Datas.empty())
		return;

	BarbaBuffer* data = Datas.head();

	//send data in queue
	std::string tag = _T("BarbaTunnelComm;");
	BarbaBuffer buf;
	buf.append((BYTE*)tag.data(), tag.size());
	buf.append((BYTE*)&NextId, sizeof DWORD);
	buf.append((BYTE)0);
	buf.append(data);
	Courier->Log3(_T("Sending DataControl. %d bytes, Id: %d"), data->size(), NextId); 
	Courier->SendData(&buf);
}

// *************** BarbaCourierDatagram Implementation
BarbaCourierDatagram::BarbaCourierDatagram(CreateStrcut* cs)
{
	_CreateStruct = cs;
	_SessionId = 0;
	LastMessageId = 0;
	LastCleanTimeoutMessagesTime = 0;
	DataControlManager.Courier = this;
	Messages.reserve(MaxMessagesCount);
}

BarbaCourierDatagram::~BarbaCourierDatagram(void)
{
	for (int i=0; i<Messages.size(); i++)
		delete Messages[i];

	delete _CreateStruct;
}

void BarbaCourierDatagram::Init()
{
}

void BarbaCourierDatagram::Log2(LPCTSTR format, ...) { va_list argp; va_start(argp, format); LogImpl(2, format, argp); va_end(argp); }
void BarbaCourierDatagram::Log3(LPCTSTR format, ...) { va_list argp; va_start(argp, format); LogImpl(3, format, argp); va_end(argp); }
void BarbaCourierDatagram::LogImpl(int level, LPCTSTR format, va_list _ArgList)
{
	TCHAR* msg = new TCHAR[1000];
	_vstprintf_s(msg, 1000, format, _ArgList);

	TCHAR* msg2 = new TCHAR[1000];
	_stprintf_s(msg2, 1000, _T("BarbaCourierDatagram: SessionId: %x, %s"), GetSessionId(), msg);

	va_list emptyList = {0};
	BarbaLogImpl(level, msg2, emptyList);
	delete msg;
	delete msg2;
}


DWORD BarbaCourierDatagram::GetNewMessageId()
{
	return ++LastMessageId;
}

// Control (1) | { MessageId (4) | TotalChunk (4) | ChunkIndex (4) } | ChunkData
void BarbaCourierDatagram::SendData(BarbaBuffer* data)
{
	DoTimer(); //good place for timer check

	//prevent fix maxSize
	size_t maxChunkSize = GetCreateStruct()->MaxChunkSize;
	size_t maxSize = maxChunkSize - BarbaUtils::GetRandom(0, (u_int)maxChunkSize/8); //prevent fixed lengh max chunk size
	Message message(GetNewMessageId(), data, (DWORD)maxSize);

	for (int i=0; i<message.Chunks.size(); i++)
	{
		BarbaBuffer chunk;
		chunk.reserve(13 +  message.Chunks[i]->size());

		BYTE controlChar = message.Chunks.size()==1 ? 1 : 4;
		chunk.append(controlChar); //control character

		//add MessageId (4) | TotalChunk (4) | ChunkIndex (4) if first 2 bit of controlChar is 4
		if ( (controlChar & 0x4)==4 )
		{
			chunk.append(&message.Id, sizeof DWORD); //MessageId
			DWORD chunksCount = (DWORD)message.Chunks.size();
			chunk.append(&chunksCount, sizeof DWORD); //TotalChunk
			chunk.append(&i, sizeof DWORD); //ChunkIndex
		}
		chunk.append(message.Chunks[i]); //data
		SendChunkToOutbound(&chunk);
	}
}

// Control (1) | { MessageId (4) | TotalChunk (4) | ChunkIndex (4) } | ChunkData
void BarbaCourierDatagram::SendChunkToInbound(BarbaBuffer* data)
{
	DoTimer(); //good place for timer check

	if (data->size()<13)
		return; //invalid size

	//restore chunk
	int offset = 0;
	BYTE* buffer = data->data();
	BYTE control = *(BYTE*)(buffer + offset);
	offset+=1;

	DWORD messageId = 0;
	DWORD chunksCount = 1;
	DWORD chunkIndex = 0;

	if ( (control & 0x4) == 4 )
	{
		messageId = *(DWORD*)(buffer + offset);
		offset+=4;

		chunksCount = *(DWORD*)(buffer + offset);
		offset+=4;

		chunkIndex = *(DWORD*)(buffer + offset);
		offset+=4;
	}

	//validate incoming data
	if (chunksCount>MaxMessageChunksCount) 
	{
		Log2(_T("Dropping chuck with invalid chunks count!"));
		return; //message cound not be more than 10000 chunk
	}
	if (chunkIndex>=chunksCount)
	{
		Log2(_T("Dropping chuck with invalid chunk index!"));
		return;
	}

	//find in messages by id
	Message* message = NULL;
	int messageIndex = -1;
	for (int i=0; i<Messages.size(); i++)
	{
		if (Messages[i]->Id==messageId)
		{
			messageIndex = i;
			message = Messages[i];
			break;
		}
	}

	//create message if not found
	if (message==NULL)
		message = new Message(messageId, chunksCount);

	//add chunk to message
	message->AddChunk(chunkIndex, buffer + offset, data->size() - offset);

	//send message if completed
	if (message->IsCompleted())
	{
		//signal end user that new data arrived
		BarbaBuffer buf;
		message->GetData(&buf);

		//check control command
		if (!DataControlManager.CheckDataControl(&buf))
		{
			Log3(_T("Receiving %d bytes."), buf.size()); 
			if (!PreReceiveData(&buf))
				ReceiveData(&buf);
		}

		//delete message
		if (messageIndex!=-1)
			RemoveMessage(messageIndex);
		delete message;
	}
	//add uncompleted message to waiting list if not exists yet
	else if (messageIndex==-1 && Messages.size()<MaxMessagesCount)
	{
		Messages.append(message);
	}

	//clear timeout messages
	RemoveTimeoutMessages();
}

bool BarbaCourierDatagram::PreReceiveData(BarbaBuffer* data)
{
	UNREFERENCED_PARAMETER(data);
	return false;
}

bool BarbaCourierDatagram::PreReceiveDataControl(BarbaBuffer* data)
{
	UNREFERENCED_PARAMETER(data);
	return false;
}

void BarbaCourierDatagram::ReceiveData(BarbaBuffer* data)
{
	UNREFERENCED_PARAMETER(data);
}

void BarbaCourierDatagram::ReceiveDataControl(BarbaBuffer* data)
{
	UNREFERENCED_PARAMETER(data);
}

void BarbaCourierDatagram::RemoveMessage(int messageIndex)
{
	size_t lastIndex = Messages.size()-1;
	Messages[messageIndex] = Messages[lastIndex];
	Messages.resize(lastIndex);
}

void BarbaCourierDatagram::RemoveTimeoutMessages()
{
	//Use abs because GetTickCount is DWORD and just work for 50 days; servers may work beyound
	if (BarbaUtils::GetTickDiff(LastCleanTimeoutMessagesTime) < GetCreateStruct()->MessageTimeout)
		return;
	LastCleanTimeoutMessagesTime = GetTickCount();

	for (int i=0; i<Messages.size(); i++)
	{
		if (BarbaUtils::GetTickDiff(Messages[i]->LastUpdateTime)>GetCreateStruct()->MessageTimeout)
		{
			Log3(_T("Dropping timeout packet. MessageChunkId: %d"), Messages[i]->Id);
			delete Messages[i];
			RemoveMessage(i);
			i--;
		}
	}
}

void BarbaCourierDatagram::SendDataControl(BarbaBuffer* data)
{
	DataControlManager.Send(data);
}

void BarbaCourierDatagram::DoTimer()
{
	if (BarbaUtils::GetTickDiff(LastTimerTime)<500)
		return;
	LastTimerTime = GetTickCount();
	Timer();
}

void BarbaCourierDatagram::Timer()
{
	DataControlManager.Process();
}
