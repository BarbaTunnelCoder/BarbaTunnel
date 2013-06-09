#include "stdafx.h"
#include "BarbaCourierDatagram.h"
#include "BarbaException.h"
#include "BarbaUtils.h"

BarbaCourierDatagram::Message::Message(DWORD id, DWORD totalParts)
{
	AddedPartCount = 0;
	Id = id;
	Parts.assign(totalParts);
	LastUpdateTime = GetTickCount(); 
}

BarbaCourierDatagram::Message::Message(DWORD id, BarbaBuffer* data, DWORD maxPartSize)
{
	Construct(id, data->data(), data->size(), maxPartSize);
}

BarbaCourierDatagram::Message::Message(DWORD id, BYTE* data, size_t dataSize, DWORD maxPartSize)
{
	Construct(id, data, dataSize, maxPartSize);
}

void BarbaCourierDatagram::Message::Construct(DWORD id, BYTE* data, size_t dataSize, DWORD maxPartSize)
{
	AddedPartCount = 0;
	Id = id;
	DWORD totalParts = (DWORD) (dataSize / maxPartSize);
	if ( (dataSize % maxPartSize)!=0 ) totalParts++;
	
	Parts.assign( totalParts );
	for (int i=0; i<(int)totalParts; i++)
		AddPart(i, data + (i*maxPartSize), min(dataSize-(i*maxPartSize), maxPartSize));

	LastUpdateTime = GetTickCount(); 
}

void BarbaCourierDatagram::Message::AddPart(DWORD partIndex, BYTE* data, size_t dataSize)
{
	if (partIndex>=Parts.size())
		throw new BarbaException(_T("PartIndex is out of range!"));
	if ( Parts[partIndex]==NULL ) AddedPartCount++;
	Parts[partIndex] = new BarbaBuffer(data, dataSize);
	LastUpdateTime = GetTickCount(); 
}

bool BarbaCourierDatagram::Message::IsCompleted()
{
	return AddedPartCount==Parts.size();
}

void BarbaCourierDatagram::Message::GetData(BarbaBuffer* data)
{
	//calculate data size for reservation
	size_t totalSize = 0;
	for (size_t i=0; i<Parts.size(); i++)
		totalSize += Parts[i]->size();
	data->reserve(totalSize);

	//append data
	for (size_t i=0; i<Parts.size(); i++)
		data->append(Parts[i]);
}

BarbaCourierDatagram::Message::~Message()
{
	for (size_t i=0; i<Parts.size(); i++)
		delete Parts[i];
}

BarbaCourierDatagram::BarbaCourierDatagram(CreateStrcut* cs)
{
	LastMessageId = 0;
	_CreateStruct = cs;
	Messages.reserve(100000); //reserver for 100K messages!
}

DWORD BarbaCourierDatagram::GetNewMessageId()
{
	return ++LastMessageId;
}


BarbaCourierDatagram::~BarbaCourierDatagram(void)
{
	for (int i=0; i<Messages.size(); i++)
		delete Messages[i];
}

// Control (1) | MessageId (4) | TotalPart (4) | PartIndex (4) | PartData
void BarbaCourierDatagram::SendData(BarbaBuffer* data)
{
	//prevent fix maxSize
	size_t maxSize = GetCreateStruct()->MaxPacketSize - BarbaUtils::GetRandom(0, (u_int)GetCreateStruct()->MaxPacketSize/8);
	Message message(GetNewMessageId(), data, (DWORD)maxSize);

	for (int i=0; i<message.Parts.size(); i++)
	{
		BarbaBuffer chunk;
		chunk.reserve(13 +  message.Parts[i]->size());
		
		chunk.append((BYTE)0); //control character
		chunk.append(&message.Id, sizeof DWORD); //MessageId
		DWORD partsCount = (DWORD)message.Parts.size();
		chunk.append(&partsCount, sizeof DWORD); //TotalPart
		chunk.append(&i, sizeof DWORD); //PartIndex
		chunk.append(message.Parts[i]); //data
		SendChunkToOutbound(&chunk);
	}
}

// Control (1) | MessageId (4) | TotalPart (4) | PartIndex (4) | PartData
void BarbaCourierDatagram::SendChunkToInbound(BarbaBuffer* data)
{
	if (data->size()<13)
		return; //invalid size
	
	//restore chunk
	int offset = 0;
	BYTE* buffer = data->data();
	BYTE control = *(BYTE*)(buffer + offset);
	offset+=1;
	UNREFERENCED_PARAMETER(control); //not used yet

	DWORD messageId = *(DWORD*)(buffer + offset);
	offset+=4;

	DWORD partsCount = *(DWORD*)(buffer + offset);
	offset+=4;

	DWORD partIndex = *(DWORD*)(buffer + offset);
	offset+=4;

	//validate incoming data
	if (partsCount>10000) return; //message cound not be more than 10000 chunk
	if (partIndex>=partsCount) return;
	
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
		message = new Message(messageId, partsCount);

	//add part to message
	message->AddPart(partIndex, buffer + offset, data->size() - offset);

	//send message if completed
	if (message->IsCompleted())
	{
		//signal end user that new data arrived
		BarbaBuffer buf;
		message->GetData(&buf);
		ReceiveData(&buf);
		
		//delete message
		if (messageIndex!=-1)
			RemoveMessage(messageIndex);
		delete message;
	}
	//add uncompleted message to waiting list if not exists yet
	else if (messageIndex==-1)
	{
		Messages.append(message);
	}

	//clear timeout messages
	RemoveTimeoutMessages();
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
	DWORD currentTime = GetTickCount();
	if (abs((long)(currentTime-LastCleanTimeoutMessagesTime))<GetCreateStruct()->MessageTimeout)
		return;

	LastCleanTimeoutMessagesTime = currentTime;
	for (int i=0; i<Messages.size(); i++)
	{
		if (abs((long)(currentTime - Messages[i]->LastUpdateTime))>GetCreateStruct()->MessageTimeout)
		{
			delete Messages[i];
			RemoveMessage(i);
			i--;
		}
	}
}


