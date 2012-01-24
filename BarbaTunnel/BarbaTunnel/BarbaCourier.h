#pragma once
#include "General.h"
#include "BarbaSocket.h"
#include "SimpleEvent.h"
#include "SimpleSafeList.h"

#define BarbaCourier_MaxMessageLength  1600
#define BarbaCourier_MaxFileHeaderSize (200*1000) //100KB

//BarbaCourierCreateStrcut
struct BarbaCourierCreateStrcut
{
	u_int SessionId;
	u_short MaxConnection;
	std::tstring RequestDataKeyName;
	std::tstring FakeHttpGetTemplate;
	std::tstring FakeHttpPostTemplate;
	std::tstring HostName;
	u_int FakeFileMaxSize;
	u_short FakePacketMinSize;
	u_int ThreadsStackSize;
	u_int ConnectionTimeout;
	u_int KeepAliveInterval;
};

//BarbaCourier
class BarbaCourier
{
protected:
	class Message
	{
	public:
		Message(BYTE* buffer, size_t count)
		{
			memcpy_s(this->Buffer, _countof(this->Buffer), buffer, count); 
			this->Count=count;
		}
		BYTE Buffer[BarbaCourier_MaxMessageLength];
		size_t Count;
	};

public:
	//@maxConnenction number of simultaneous connection for each outgoing and incoming, eg: 2 mean 2 connection for send and 2 connection for receive so the total will be 4
	explicit BarbaCourier(BarbaCourierCreateStrcut* cs);
	virtual void Send(BYTE* buffer, size_t bufferCount);
	virtual void Receive(BYTE* buffer, size_t bufferCount);
	virtual void GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, std::vector<BYTE>* fakeFileHeader, bool createNew);
	virtual void Crypt(BYTE* data, size_t dataLen, bool encrypt);
	virtual bool IsServer()=0;
	std::tstring GetRequestDataFromHttpRequest(LPCTSTR httpRequest);
	size_t GetSentBytesCount() {return this->SentBytesCount;}
	size_t GetReceiveBytesCount() {return this->ReceivedBytesCount;}
		
	//Call this method to delete object, This method will signal all created thread to finish their job
	//This method will call asynchronously. do not use the object after call it 
	//@return the handle for deleting thread
	HANDLE Delete();

private:
	std::tstring PrepareFakeRequests(std::tstring* request);
	//@return false if max connection reached
	static void CloseSocketsList(SimpleSafeList<BarbaSocket*>* list);
	size_t MaxMessageBuffer;
	size_t SentBytesCount;
	size_t ReceivedBytesCount;
	SimpleEvent SendEvent;
	SimpleSafeList<Message*> Messages;
	static unsigned int __stdcall DeleteThread(void* object);

protected:
	void Log(LPCTSTR format, ...);
	virtual void Dispose();
	bool IsDisposing();
	virtual ~BarbaCourier(void);
	virtual void Send(Message* message, bool highPriority=false);
	void Sockets_Add(BarbaSocket* socket, bool isOutgoing);
	void Sockets_Remove(BarbaSocket* socket, bool isOutgoing);
	void ProcessIncoming(BarbaSocket* barbaSocket);
	void ProcessOutgoing(BarbaSocket* barbaSocket, size_t maxBytes=0);
	void SendFakeFileHeader(BarbaSocket* socket, std::vector<BYTE>* fakeFileHeader);
	void WaitForIncomingFakeHeader(BarbaSocket* socket, LPCTSTR httpRequest);
	void InitFakeRequestVars(std::tstring& src, LPCTSTR filename, LPCTSTR contentType, size_t fileSize, size_t fileHeaderSize);
	volatile DWORD LastReceivedTime;
	volatile DWORD LastSentTime;

	SimpleSafeList<BarbaSocket*> IncomingSockets;
	SimpleSafeList<BarbaSocket*> OutgoingSockets;
	SimpleSafeList<HANDLE> Threads;
	SimpleEvent DisposeEvent;

	BarbaCourierCreateStrcut CreateStruct;
};

