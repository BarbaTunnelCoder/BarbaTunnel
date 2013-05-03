#pragma once
#include "General.h"
#include "BarbaSocket.h"
#include "SimpleEvent.h"
#include "SimpleSafeList.h"

#define BarbaCourier_MaxMessageLength 0xFFFF
#define BarbaCourier_MaxFileHeaderSize (200*1000) //100KB

//BarbaCourierCreateStrcut
struct BarbaCourierCreateStrcut
{
	u_int SessionId;
	u_short MaxConnection;
	std::tstring RequestDataKeyName;
	std::tstring HttpGetTemplate;
	std::tstring HttpPostTemplate;
	std::tstring HttpGetTemplatePerPacket;
	std::tstring HttpPostTemplatePerPacket;
	std::tstring HostName;
	u_int FakeFileMaxSize;
	u_short FakePacketMinSize;
	u_int ThreadsStackSize;
	u_int ConnectionTimeout;
	u_int KeepAliveInterval;
	bool AllowBombard;
	std::tstring HttpBombardMode;
	std::tstring HttpGetTemplateBombard;
	std::tstring HttpPostTemplateBombard;
};

//BarbaCourier
class BarbaCourier
{
protected:
	class Message
	{
	public:
		explicit Message() { }
		explicit Message(BarbaBuffer* data)
		{
			this->Buffer.assign(data);
		}

		BYTE* GetData() {return this->Buffer.data();}
		size_t GetCount() {return this->Buffer.size();}
	
	private:
		BarbaBuffer Buffer;
	};

public:
	//@maxConnenction number of simultaneous connection for each outgoing and incoming, eg: 2 mean 2 connection for send and 2 connection for receive so the total will be 4
	explicit BarbaCourier(BarbaCourierCreateStrcut* cs);
	void RefreshParameters();
	virtual void Send(BarbaBuffer* data);
	virtual void Receive(BarbaBuffer* data);
	virtual void GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew);
	virtual void Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt);
	virtual bool IsServer()=0;
	std::tstring GetRequestDataFromHttpRequest(LPCTSTR httpRequest);
	size_t GetSentBytesCount() {return this->SentBytesCount;}
	size_t GetReceiveBytesCount() {return this->ReceivedBytesCount;}
		
	//Call this method to delete object, This method will signal all created thread to finish their job
	//This method will call asynchronously. do not use the object after call it 
	//@return the handle for deleting thread
	HANDLE Delete();

private:
	void Crypt(BarbaBuffer* data, size_t index, bool encrypt);
	std::tstring PrepareFakeRequests(std::tstring* request);
	//@return false if max connection reached
	static void CloseSocketsList(SimpleSafeList<BarbaSocket*>* list);
	static unsigned int __stdcall DeleteThread(void* object);
	size_t MaxMessageBuffer;
	size_t SentBytesCount;
	size_t ReceivedBytesCount;
	SimpleEvent SendEvent;
	SimpleSafeList<Message*> Messages;

protected:
	void Log(LPCTSTR format, ...);
	virtual void Dispose();
	bool IsDisposing() { return this->DisposeEvent.IsSet(); }
	virtual ~BarbaCourier(void);
	virtual void Send(Message* message, bool highPriority=false);
	void Sockets_Add(BarbaSocket* socket, bool isOutgoing);
	void Sockets_Remove(BarbaSocket* socket, bool isOutgoing);
	void ProcessIncoming(BarbaSocket* barbaSocket, size_t maxBytes=0);
	void ProcessOutgoing(BarbaSocket* barbaSocket, size_t maxBytes=0);
	virtual void BeforeSendMessage(BarbaSocket* barbaSocket, size_t messageLength);
	virtual void AfterSendMessage(BarbaSocket* barbaSocket);
	virtual void BeforeReceiveMessage(BarbaSocket* barbaSocket);
	virtual void AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength);
	void SendFakeFileHeader(BarbaSocket* socket, BarbaBuffer* fakeFileHeader);
	void WaitForIncomingFakeHeader(BarbaSocket* socket, size_t fileHeaderSize);
	void InitRequestVars(std::tstring& src, LPCTSTR filename, LPCTSTR contentType, size_t fileSize, size_t fileHeaderSize);
	std::tstring GetFakeRequest(bool httpPost, bool bombard);
	volatile DWORD LastReceivedTime;
	volatile DWORD LastSentTime;
	bool IsBombardGet; 
	bool IsBombardPost;  
	bool IsBombardPostReply;  

	SimpleSafeList<BarbaSocket*> IncomingSockets;
	SimpleSafeList<BarbaSocket*> OutgoingSockets;
	SimpleSafeList<HANDLE> Threads;
	SimpleEvent DisposeEvent;

	BarbaCourierCreateStrcut CreateStruct;
};

