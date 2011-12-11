#pragma once
#include "BarbaSocket.h"
#include "SimpleEvent.h"
#include "SimpleSafeList.h"


//BarbaCourier
class BarbaCourier
{
protected:
	class Message
	{
	public:
		Message(BYTE* buffer, size_t count)
		{
			this->Buffer = new BYTE[count]; 
			memcpy_s(this->Buffer, count, buffer, count); 
			this->Count=count;
		}
		~Message() {delete this->Buffer;}
		BYTE* Buffer;
		size_t Count;
	};

public:
	size_t MaxSendMessageBuffer;
	//@maxConnenction number of simultaneous connection for each outgoing and incoming, eg: 2 mean 2 connection for send and 2 connection for receive so the total will be 4
	explicit BarbaCourier(u_short maxConnenction);
	virtual void Send(BYTE* buffer, size_t bufferCount);
	virtual void Receive(BYTE* buffer, size_t bufferCount);
	void InitFakeRequests(LPCSTR httpGetTemplate, LPCSTR httpPostTemplate);
	
	//Call this method to delete object, This method will signal all created thread to finish their job
	//This method will call asynchronously. do not use the object after call it 
	void Delete();

private:
	unsigned int SentBytesCount;
	unsigned int ReceiveBytesCount;
	SimpleEvent SendEvent;
	SimpleSafeList<Message*> Messages;
	static unsigned int __stdcall DeleteThread(void* object);

protected:
	virtual void Dispose();
	bool IsDisposing();
	virtual ~BarbaCourier(void);
	virtual void Send(Message* message, bool highPriority=false);
	void ProcessIncoming(BarbaSocket* barbaSocket);
	void ProcessOutgoing(BarbaSocket* barbaSocket);
	SimpleSafeList<BarbaSocket*> Sockets;
	SimpleSafeList<HANDLE> Threads;
	size_t MaxConnection;
	SimpleEvent DisposeEvent;

	std::string FakeHttpGetTemplate;
	std::string FakeHttpPostTemplate;
};

//BarbaCourierServer
class BarbaCourierServer : public BarbaCourier
{
private:
	//used to pass data to created thread
	struct ServerThreadData
	{
		ServerThreadData(BarbaCourier* courier, BarbaSocket* socket, bool IsOutgoing) { this->Courier=courier; this->Socket=socket; this->IsOutgoing = IsOutgoing; }
		BarbaCourier* Courier;
		BarbaSocket* Socket;
		bool IsOutgoing;
	};

public:
	BarbaCourierServer(u_short maxConnenction);
	virtual ~BarbaCourierServer(void);
	//@return false if no new connection accepted and caller should delete the socket
	//@return true if connection accepted, in this case caller should not delete the socket and it will be deleted automatically
	bool AddSocket(BarbaSocket* Socket, bool isOutgoing);

private:
	void SendFakeReply(BarbaSocket* barbaSocket, bool postReply);
	static unsigned int __stdcall WorkerThread(void* serverThreadData);
};


//BarbaCourierClient
class BarbaCourierClient : public BarbaCourier
{
private:
	//used to pass data to created thread
	struct ClientThreadData
	{
		ClientThreadData(BarbaCourier* courier, bool isOutgoing) {this->Courier=courier; this->IsOutgoing = isOutgoing;}
		BarbaCourier* Courier;
		bool IsOutgoing;
	};

public:
	void SendFakeRequest(BarbaSocket* barbaSocket, bool isOutgoing);
	bool WaitForFakeReply(BarbaSocket* barbaSocket);
	BarbaCourierClient(DWORD remoteIp, u_short remotePort, u_short maxConnenction);
	virtual ~BarbaCourierClient();

private:
	DWORD RemoteIp;
	u_short RemotePort;
	static unsigned int __stdcall WorkerThread(void* clientThreadData);
};
