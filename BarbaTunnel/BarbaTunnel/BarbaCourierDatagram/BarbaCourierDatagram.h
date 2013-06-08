#pragma once
#include "BarbaBuffer.h"

class BarbaCourierDatagram
{
public:
	class CreateStrcut
	{
	public:
		CreateStrcut() { SessionId=0; MaxPacketSize=0; }
		DWORD SessionId;
		size_t MaxPacketSize;
	};

protected:
	class Message
	{
	public:
		Message(DWORD id, DWORD totalParts);
		Message(DWORD id, BarbaBuffer* data, DWORD maxPartSize);
		Message(DWORD id, BYTE* data, size_t dataSize, DWORD maxPartSize);
		void AddPart(DWORD partIndex, BYTE* data, size_t dataSize);
		bool IsCompleted(); //return true if message contains all parts
		void GetData(BarbaBuffer* data); //merge all parts and return data
		~Message();

		DWORD Id; 
		DWORD LastUpdateTime;
		BarbaArray<BarbaBuffer*> Parts;

	private:
		void Construct(DWORD id, BYTE* data, size_t dataSize, DWORD maxPartSize);
		DWORD AddedPartCount;
	};


public:
	BarbaCourierDatagram(void);
	virtual ~BarbaCourierDatagram(void);

	void SendData(BarbaBuffer* data);
	virtual void ReceiveData(BarbaBuffer* data)=0;
	CreateStrcut* GetCreateStruct() {return _CreateStruct;}

protected:
	DWORD GetNewMessageId();
	virtual void SendMessage(Message* message)=0; //Subclasser should send the message
	void ReceiveMessage(Message* data); //Subclasser should called it when got new message

private:
	CreateStrcut* _CreateStruct;
};

