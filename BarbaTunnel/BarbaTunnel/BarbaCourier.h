#pragma once
#include "General.h"
#include "BarbaSocket.h"
#include "SimpleEvent.h"
#include "SimpleSafeList.h"

#define BarbaCourier_MaxMessageLength 0xFFFF
#define BarbaCourier_MaxFileHeaderSize (200*1000) //100KB

//BarbaCourier
class BarbaCourier
{
public:
	struct CreateStrcutBag
	{
		u_int SessionId;
		u_short MaxConnection;
		std::tstring RequestDataKeyName;
		std::tstring HostName;
		u_int FakeFileMaxSize;
		u_short FakePacketMinSize;
		u_int ThreadsStackSize;
		u_int ConnectionTimeout;
		u_int KeepAliveInterval;
		bool AllowBombard;
		std::tstring HttpBombardMode;
	};

protected:
	class Message
	{
	public:
		explicit Message() { }
		explicit Message(BarbaBuffer* data)
		{
			Buffer.assign(data);
		}

		BYTE* GetData() {return Buffer.data();}
		size_t GetDataSize() {return Buffer.size();}
	
	private:
		BarbaBuffer Buffer;
	};

public:
	//@maxConnenction number of simultaneous connection for each outgoing and incoming, eg: 2 mean 2 connection for send and 2 connection for receive so the total will be 4
	explicit BarbaCourier(CreateStrcutBag* cs);
	virtual void RefreshParameters();
	virtual void Send(BarbaBuffer* data);
	virtual void Receive(BarbaBuffer* data);
	virtual void Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt);
	virtual bool IsServer()=0;
	size_t GetSentBytesCount() {return this->SentBytesCount;}
	size_t GetReceiveBytesCount() {return this->ReceivedBytesCount;}
		
	//Call this method to delete object, This method will signal all created thread to finish their job
	//This method will call asynchronously. do not use the object after call it 
	//@return the handle for deleting thread
	HANDLE Delete();
	
private:
	//@return false if max connection reached
	static void CloseSocketsList(SimpleSafeList<BarbaSocket*>* list);
	static unsigned int __stdcall DeleteThread(void* object);
	size_t MaxMessageBuffer;
	SimpleEvent SendEvent;
	SimpleCriticalSection SendEventCS;
	SimpleSafeList<Message*> Messages;

protected: 
	void Crypt(BarbaBuffer* data, size_t index, bool encrypt);
	void GetMessages(BarbaArray<Message*>& messages);
	Message* GetMessage();

protected:
	void LogImpl(int level, LPCTSTR format, va_list _ArgList);
	void Log2(LPCTSTR format, ...);
	void Log3(LPCTSTR format, ...);
	virtual void Dispose();
	bool IsDisposing() { return this->DisposeEvent.IsSet(); }
	virtual ~BarbaCourier(void);
	virtual void Send(BarbaArray<Message*>& messages, bool highPriority=false);
	virtual void Send(Message* message, bool highPriority=false);
	void Sockets_Add(BarbaSocket* socket, bool isOutgoing);
	void Sockets_Remove(BarbaSocket* socket, bool isOutgoing);
	void ProcessIncoming(BarbaSocket* barbaSocket, size_t maxBytes);
	void ProcessOutgoing(BarbaSocket* barbaSocket, size_t maxBytes);
	size_t ProcessIncomingMessage(BarbaSocket* barbaSocket, size_t cryptIndex, size_t chunkSize);
	void ProcessOutgoingMessages(BarbaArray<Message*>& messages, size_t cryptIndex, size_t maxPacketSize, BarbaBuffer* packet);
	virtual void BeforeSendMessage(BarbaSocket* barbaSocket, BarbaBuffer* messageBuffer);
	virtual void AfterSendMessage(BarbaSocket* barbaSocket, bool isTransferFinished);
	virtual void BeforeReceiveMessage(BarbaSocket* barbaSocket, size_t* chunkSize);
	virtual void AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength, bool isTransferFinished);
	std::tstring CreateRequestParam(size_t transferSize, bool outgoing, LPCTSTR other);
	volatile DWORD LastReceivedTime;
	volatile DWORD LastSentTime;
	volatile size_t SentBytesCount;
	volatile size_t ReceivedBytesCount;

	SimpleSafeList<BarbaSocket*> IncomingSockets;
	SimpleSafeList<BarbaSocket*> OutgoingSockets;
	SimpleSafeList<HANDLE> Threads;
	SimpleEvent DisposeEvent;
	CreateStrcutBag CreateStruct;
};

