#include "StdAfx.h"
#include "BarbaCourier.h"

BarbaCourier::BarbaCourier(u_short maxConnenction)
	: SendEvent(true, true)
	, DisposeEvent(true, false)
{
	this->MaxSendMessageBuffer = INFINITE;
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

void BarbaCourier::Delete()
{
	_beginthreadex(0, 128 ,DeleteThread, (void*)this,0, NULL);
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
		thread = this->Threads.RemoveHead();
	}
}


void BarbaCourier::Send(BYTE* buffer, size_t bufferCount)
{
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
	if (Messages.GetCount()>MaxSendMessageBuffer)
		Messages.RemoveHead();

	SendEvent.Set();
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
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
	replaceAll(FakeHttpGetTemplate, r, e);
	replaceAll(FakeHttpGetTemplate, n, rn);

	FakeHttpPostTemplate.append(n);
	FakeHttpPostTemplate.append(n);
	replaceAll(FakeHttpPostTemplate, r, e);
	replaceAll(FakeHttpPostTemplate, n, rn);
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
				//try to send message
				int sentCount = barbaSocket->Send(message->Buffer, message->Count);
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
			BYTE buf[1000];
			int recieveCount = barbaSocket->Receive(buf, _countof(buf), true);
			if (recieveCount==0)
				break;
			this->ReceiveBytesCount += recieveCount;
			this->Receive(buf, recieveCount);
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
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, 128, WorkerThread, outgoingThreadData, 0, NULL));

		//create incoming connection thread
		ClientThreadData* incomingThreadData = new ClientThreadData(this, false);
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, 128, WorkerThread, incomingThreadData, 0, NULL));
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
			_tprintf_s(_T("New connection!\n"));
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
	_tprintf_s(_T("Thread Fin!\n"));
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
	Threads.AddTail( (HANDLE)_beginthreadex(NULL, 128, WorkerThread, (void*)threadData, 0, NULL));
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
	printf("Connection closed\n");
	return 0;
}
