#include "stdafx.h"
#include "BarbaCourierDatagram.h"
#include "BarbaException.h"


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

BarbaCourierDatagram::BarbaCourierDatagram(void)
{
}


BarbaCourierDatagram::~BarbaCourierDatagram(void)
{
}

void BarbaCourierDatagram::SendData(BarbaBuffer* data)
{
	UNREFERENCED_PARAMETER(data);
}
