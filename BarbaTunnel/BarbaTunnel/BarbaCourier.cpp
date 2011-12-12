#include "StdAfx.h"
#include "BarbaCourier.h"
#include "StringUtil.h"

BarbaCourier::BarbaCourier(u_short maxConnenction, size_t threadsStackSize)
	: SendEvent(true, true)
	, DisposeEvent(true, false)
{
	this->ThreadsStackSize = threadsStackSize;
	this->MaxMessageBuffer = INFINITE;
	this->SentBytesCount = 0;
	this->ReceiveBytesCount = 0;
	this->MaxConnection = maxConnenction;
}

unsigned BarbaCourier::DeleteThread(void* object) 
{
	BarbaCourier* _this = (BarbaCourier*)object;
	_this->Dispose();
	delete _this; 
	return 0;
}

HANDLE BarbaCourier::Delete()
{
	return (HANDLE)_beginthreadex(0, this->ThreadsStackSize, DeleteThread, (void*)this, 0, NULL);
}


BarbaCourier::~BarbaCourier(void)
{
}

bool BarbaCourier::IsDisposing()
{
	return DisposeEvent.Wait(0)==WAIT_OBJECT_0;
}

void BarbaCourier::Dispose()
{
	DisposeEvent.Set();
	SimpleLock lock(this->Sockets.GetCriticalSection());
	size_t count = this->Sockets.GetCount();
	BarbaSocket** socketsArray = new BarbaSocket*[count];
	this->Sockets.GetAll(socketsArray, &count);
	for (size_t i=0; i<count; i++)
	{
		try
		{
			socketsArray[i]->Close();
		}
		catch(...)
		{
		}
	}
	lock.Unlock();
	delete socketsArray;

	//delete all messages
	Message* message = this->Messages.RemoveHead();
	while(message!=NULL)
	{
		delete message;
		message = this->Messages.RemoveHead();
	}

	//pulse all send thread to end
	SendEvent.Set();

	//wait for all thread to end
	HANDLE thread = this->Threads.RemoveHead();
	while (thread!=NULL)
	{
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		thread = this->Threads.RemoveHead();
	}
}


void BarbaCourier::Send(BYTE* buffer, size_t bufferCount)
{
	if (bufferCount>BarbaCourier_MaxMessageLength)
		throw _T("Message is too big!");
	Send(new Message(buffer, bufferCount));
}

void BarbaCourier::Send(Message* message, bool highPriority)
{
	SimpleLock lock(this->Messages.GetCriticalSection());
	if (highPriority)
		Messages.AddHead(message);
	else
		Messages.AddTail(message);

	//check maximum send buffer
	if (Messages.GetCount()>MaxMessageBuffer)
		Messages.RemoveHead();

	SendEvent.Set();
}


void BarbaCourier::InitFakeRequests(LPCSTR httpGetRequest, LPCSTR httpPostRequest)
{
	FakeHttpGetTemplate = httpGetRequest;
	FakeHttpPostTemplate = httpPostRequest;

	//fix next-line
	std::string n = "\n";
	std::string r = "\r";
	std::string e = "";
	std::string rn = "\r\n";

	FakeHttpGetTemplate.append(n);
	FakeHttpGetTemplate.append(n);
	StringUtil::ReplaceAll(FakeHttpGetTemplate, r, e);
	StringUtil::ReplaceAll(FakeHttpGetTemplate, n, rn);

	FakeHttpPostTemplate.append(n);
	FakeHttpPostTemplate.append(n);
	StringUtil::ReplaceAll(FakeHttpPostTemplate, r, e);
	StringUtil::ReplaceAll(FakeHttpPostTemplate, n, rn);
}

void BarbaCourier::Receive(BYTE* /*buffer*/, size_t /*bufferCount*/)
{
}


void BarbaCourier::ProcessOutgoing(BarbaSocket* barbaSocket)
{
	//remove message from list
	while(!this->IsDisposing())
	{
		DWORD waitResult = SendEvent.Wait(2*1000);

		//check is connection still writable
		if (waitResult==WAIT_TIMEOUT && !barbaSocket->IsWritable())
			break;
		
		//reset if there is no message
		SimpleLock lock(this->Messages.GetCriticalSection());
		if (this->Messages.IsEmpty() && !this->IsDisposing())
		{
			SendEvent.Reset();
			continue;
		}
		lock.Unlock();

		//post all message
		Message* message = this->Messages.RemoveHead();
		while (message!=NULL)
		{
			try
			{
				//add message length to start of packet, then add message itself
				BYTE sendPacket[BarbaCourier_MaxMessageLength+2];
				memcpy_s(sendPacket, 2, &message->Count, 2);
				memcpy_s(sendPacket+2, BarbaCourier_MaxMessageLength, &message, message->Count);

				//try to send packet [Len+Message]
				int sentCount = barbaSocket->Send(sendPacket, message->Count+2);
				this->SentBytesCount += sentCount;
				
				//delete sent message
				delete message;
				
				//get next message
				message = this->Messages.RemoveHead();
			}
			catch(...)
			{
				//back message to list if failed
				Send(message, true);

				//don't continue if this socket has error
				return;
			}
			
		}
	}
}

void BarbaCourier::ProcessIncoming(BarbaSocket* barbaSocket)
{
	//remove message from list
	while(!this->IsDisposing())
	{
		try
		{
			u_short messageLen = 0;
			if (barbaSocket->Receive((BYTE*)&messageLen, 2, true)!=2)
				break;

			//check received message len
			if (messageLen>BarbaCourier_MaxMessageLength)
				throw _T("Out of sync!");

			//read message
			BYTE messageBuf[BarbaCourier_MaxMessageLength];
			int receiveCount = barbaSocket->Receive(messageBuf, messageLen, true);
			if (receiveCount!=messageLen)
				throw _T("Out of sync!");

			this->ReceiveBytesCount += receiveCount + 2;
			this->Receive(messageBuf, receiveCount);
		}
		catch (...)
		{
			break;
		}
	}
}


BarbaCourierClient::BarbaCourierClient(DWORD remoteIp, u_short remotePort, u_short maxConnenction)
	: BarbaCourier(maxConnenction)
{
	this->RemoteIp = remoteIp;
	this->RemotePort = remotePort;

	for (u_short i=0; i<maxConnenction; i++)
	{
		//create outgoing connection thread
		ClientThreadData* outgoingThreadData = new ClientThreadData(this, true);
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->ThreadsStackSize, WorkerThread, outgoingThreadData, 0, NULL));

		//create incoming connection thread
		ClientThreadData* incomingThreadData = new ClientThreadData(this, false);
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->ThreadsStackSize, WorkerThread, incomingThreadData, 0, NULL));
	}
}

BarbaCourierClient::~BarbaCourierClient()
{
}

void BarbaCourierClient::SendFakeRequest(BarbaSocket* barbaSocket, bool isOutgoing)
{
	std::string fakeString = isOutgoing ? FakeHttpPostTemplate : FakeHttpGetTemplate;
	barbaSocket->Send((BYTE*)fakeString.data(), fakeString.length());
}

bool BarbaCourierClient::WaitForFakeReply(BarbaSocket* barbaSocket)
{
	std::string header = barbaSocket->ReadHttpHeader();
	return !header.empty();
}


unsigned int BarbaCourierClient::WorkerThread(void* clientThreadData)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	ClientThreadData* threadData = (ClientThreadData*)clientThreadData;
	BarbaCourierClient* _this = (BarbaCourierClient*)threadData->Courier;
	bool isOutgoing = threadData->IsOutgoing;

	BarbaSocketClient* socket = NULL;
	while (!_this->IsDisposing())
	{
		try
		{
			//create socket
			_tprintf_s(_T("New connection!\n")); //check
			socket = NULL;
			socket = new BarbaSocketClient(_this->RemoteIp, _this->RemotePort);

			//add socket to store
			_this->Sockets.AddTail(socket);

			//send fake request
			_this->SendFakeRequest(socket, isOutgoing);
			
			//wait for fake reply
			if (!_this->WaitForFakeReply(socket))
				throw "No Fake Reply!";

			//process socket until socket closed
			if (isOutgoing)
				_this->ProcessOutgoing(socket);
			else
				_this->ProcessIncoming(socket);

		}
		catch (...)
		{
		}

		//delete socket
		if (socket!=NULL)
		{
			//remove socket from store
			_this->Sockets.Remove(socket);
			delete socket;
		}

		//wait for next connection
		_this->DisposeEvent.Wait(5000);
	}

	delete threadData;
	_tprintf_s(_T("Thread Fin!\n")); //check
	return 0;
}


BarbaCourierServer::BarbaCourierServer(u_short maxConnenction)
	: BarbaCourier(maxConnenction)
{
}


BarbaCourierServer::~BarbaCourierServer(void)
{
}

void BarbaCourierServer::SendFakeReply(BarbaSocket* barbaSocket, bool isOutgoing)
{
	std::string fakeString = isOutgoing ? FakeHttpGetTemplate : FakeHttpPostTemplate;
	barbaSocket->Send((BYTE*)fakeString.data(), fakeString.length());
}

bool BarbaCourierServer::AddSocket(BarbaSocket* barbaSocket, bool isOutgoing)
{
	if (this->IsDisposing())
		return false;

	if (this->Sockets.GetCount()>=this->MaxConnection*2)
		return false;

	this->Sockets.AddTail(barbaSocket);

	//create incoming connection thread
	ServerThreadData* threadData = new ServerThreadData(this, barbaSocket, isOutgoing);
	Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->ThreadsStackSize, WorkerThread, (void*)threadData, 0, NULL));
	return true;
}

unsigned int BarbaCourierServer::WorkerThread(void* serverThreadData)
{
	ServerThreadData* threadData = (ServerThreadData*)serverThreadData;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	BarbaCourierServer* _this = (BarbaCourierServer*)threadData->Courier;
	BarbaSocket* barbaSocket = (BarbaSocket*)threadData->Socket;
	bool isOutgoing = threadData->IsOutgoing;

	try
	{
		//add socket to store
		_this->Sockets.AddTail(barbaSocket);

		//reply fake request
		_this->SendFakeReply(barbaSocket, isOutgoing);

		//process socket until socket closed
		if (isOutgoing)
			_this->ProcessOutgoing(barbaSocket);
		else
			_this->ProcessIncoming(barbaSocket);
	}
	catch(...)
	{
	}

	//remove socket from store
	_this->Sockets.Remove(barbaSocket);
	delete barbaSocket;
	delete threadData;
	printf("Connection closed\n"); //check
	return 0;
}
