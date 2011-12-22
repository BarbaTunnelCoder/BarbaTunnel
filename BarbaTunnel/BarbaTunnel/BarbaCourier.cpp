#include "StdAfx.h"
#include "BarbaCourier.h"
#include "StringUtil.h"
#include "BarbaUtils.h"

BarbaCourier::BarbaCourier(u_short maxConnenction, size_t threadsStackSize)
	: SendEvent(true, true)
	, DisposeEvent(true, false)
{
	this->ThreadsStackSize = threadsStackSize;
	this->MaxMessageBuffer = INFINITE;
	this->SentBytesCount = 0;
	this->ReceivedBytesCount = 0;
	this->MaxConnection = maxConnenction;
	this->FakeMaxFileSize = 0;
}

unsigned BarbaCourier::DeleteThread(void* object) 
{
	BarbaCourier* _this = (BarbaCourier*)object;
	_this->Dispose();
	delete _this; 
	return 0;
}

void BarbaCourier::Log(LPCTSTR format, ...)
{
	va_list argp;
	va_start(argp, format);
	TCHAR msg[1000];
	_vstprintf_s(msg, format, argp);
	va_end(argp);

	TCHAR msg2[1000];
	_stprintf_s(msg2, _T("BarbaCourier: ThreadId: %4x, %s"), GetCurrentThreadId(), msg);
	BarbaLog2(msg2);
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

void BarbaCourier::CloseSocketsList(SimpleSafeList<BarbaSocket*>* list)
{
	//IncomingSockets
	SimpleSafeList<BarbaSocket*>::AutoLockBuffer autoLockBuf(list);
	BarbaSocket** socketsArray = autoLockBuf.GetBuffer();
	for (size_t i=0; i<list->GetCount(); i++)
	{
		try
		{
			socketsArray[i]->Close();
		}
		catch(BarbaException* er)
		{
			delete er;
		}
	}
	autoLockBuf.Unlock();
}

void BarbaCourier::Dispose()
{
	Log(_T("BarbaCourier disposing."));
	DisposeEvent.Set();

	CloseSocketsList(&this->IncomingSockets);
	CloseSocketsList(&this->OutgoingSockets);

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
	Log(_T("BarbaCourier disposed."));
}


void BarbaCourier::Send(BYTE* buffer, size_t bufferCount)
{
	if (bufferCount>BarbaCourier_MaxMessageLength)
		new BarbaException( _T("Message is too big to send!") );
	Send(new Message(buffer, bufferCount));
}

void BarbaCourier::Send(Message* message, bool highPriority)
{
	//wait till pulse set
	SimpleLock lock(Messages.GetCriticalSection());

	if (highPriority)
		Messages.AddHead(message);
	else
		Messages.AddTail(message);

	//check maximum send buffer
	if (Messages.GetCount()>MaxMessageBuffer)
		delete Messages.RemoveHead();

	SendEvent.Set();
}


void BarbaCourier::InitFakeRequests(LPCSTR httpGetRequest, LPCSTR httpPostRequest, size_t maxFileSize)
{
	this->FakeHttpGetTemplate = httpGetRequest;
	this->FakeHttpPostTemplate = httpPostRequest;
	this->FakeMaxFileSize = maxFileSize;

	//fix next-line
	std::string n = "\n";
	std::string r = "\r";
	std::string e = "";
	std::string rn = "\r\n";

	this->FakeHttpGetTemplate.append(n);
	this->FakeHttpGetTemplate.append(n);
	StringUtil::ReplaceAll(this->FakeHttpGetTemplate, r, e);
	StringUtil::ReplaceAll(this->FakeHttpGetTemplate, n, rn);

	this->FakeHttpPostTemplate.append(n);
	this->FakeHttpPostTemplate.append(n);
	StringUtil::ReplaceAll(this->FakeHttpPostTemplate, r, e);
	StringUtil::ReplaceAll(this->FakeHttpPostTemplate, n, rn);
}

void BarbaCourier::Receive(BYTE* /*buffer*/, size_t /*bufferCount*/)
{
}


void BarbaCourier::ProcessOutgoing(BarbaSocket* barbaSocket, size_t maxBytes)
{
	size_t sentBytes = 0;
	bool transferFinish = false;
	
	//remove message from list
	while(!this->IsDisposing() && !transferFinish)
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
			//check is file finished
			transferFinish = maxBytes!=0 && sentBytes>=maxBytes;
			if (transferFinish)
			{
				Log(_T("Finish sending fake file."));
				break;
			}

			try
			{
				//add message length to start of packet, then add message itself
				BYTE sendPacket[BarbaCourier_MaxMessageLength+2];
				memcpy_s(sendPacket, 2, &message->Count, 2);
				memcpy_s(sendPacket+2, BarbaCourier_MaxMessageLength, message->Buffer, message->Count);

				int sentCount = barbaSocket->Send(sendPacket, message->Count+2);
				this->SentBytesCount += sentCount; //courier bytes
				sentBytes += sentCount; //current socket bytes
				
				//delete sent message
				delete message;
				
				//get next message
				message = this->Messages.RemoveHead();
			}
			catch(...)
			{
				//bring back message to list if failed
				Send(message, true);
				throw;
			}
			
		}
	}
}

void BarbaCourier::ProcessIncoming(BarbaSocket* barbaSocket, size_t maxBytes)
{
	size_t receivedBytes = 0;
	bool transferFinish = false;

	//remove message from list
	while(!this->IsDisposing() && !transferFinish)
	{
		transferFinish = maxBytes!=0 && receivedBytes>=maxBytes;
		if (transferFinish)
		{
			Log(_T("Finish getting fake file."));
			break;
		}

		u_short messageLen = 0;
		if (barbaSocket->Receive((BYTE*)&messageLen, 2, true)!=2)
			break;

		//check received message len
		if (messageLen>BarbaCourier_MaxMessageLength)
			new BarbaException( _T("Out of sync while reading message length!") );

		//read message
		BYTE messageBuf[BarbaCourier_MaxMessageLength];
		int receiveCount = barbaSocket->Receive(messageBuf, messageLen, true);
		if (receiveCount!=messageLen)
			new BarbaException( _T("Out of sync while reading message!") );

		receivedBytes += receiveCount + 2; //current socket byes
		this->ReceivedBytesCount += receiveCount + 2; //courier byes
		this->Receive(messageBuf, receiveCount);
	}
}

void BarbaCourier::Sockets_Add(BarbaSocket* socket, bool isIncoming)
{
	SimpleSafeList<BarbaSocket*>* list = isIncoming ? &this->IncomingSockets : &this->OutgoingSockets;
	SimpleLock lock(list->GetCriticalSection());
	if (list->GetCount()>=this->MaxConnection)
		throw new BarbaException(_T("Reject new HTTP connection due the maximum connections. MaxUserConnection is: %d"), this->MaxConnection);
	list->AddTail(socket);
	socket->SetNoDelay(true);
	Log(_T("HTTP %s connection added. Connections Count: %d."), isIncoming ? _T("POST") : _T("GET"), list->GetCount());
}

void BarbaCourier::Sockets_Remove(BarbaSocket* socket, bool isIncoming)
{
	SimpleSafeList<BarbaSocket*>* list = isIncoming ? &this->IncomingSockets : &this->OutgoingSockets;
	list->Remove(socket);
	delete socket;
	Log(_T("HTTP connection removed."));
}


BarbaCourierClient::BarbaCourierClient(u_short maxConnenction, DWORD remoteIp, u_short remotePort, LPCSTR fakeHttpGetTemplate, LPCSTR fakeHttpPostTemplate)
	: BarbaCourier(maxConnenction)
{
	this->RemoteIp = remoteIp;
	this->RemotePort = remotePort;
	this->InitFakeRequests(fakeHttpGetTemplate, fakeHttpPostTemplate, 15000000);

	for (u_short i=0; i<maxConnenction; i++)
	{
		//create outgoing connection thread
		ClientThreadData* outgoingThreadData = new ClientThreadData(this, true);
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->ThreadsStackSize, ClientWorkerThread, outgoingThreadData, 0, NULL));

		//create incoming connection thread
		ClientThreadData* incomingThreadData = new ClientThreadData(this, false);
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->ThreadsStackSize, ClientWorkerThread, incomingThreadData, 0, NULL));
	}
}

BarbaCourierClient::~BarbaCourierClient()
{
}

void BarbaCourierClient::SendFakeRequest(BarbaSocket* barbaSocket, bool isOutgoing)
{
	std::string fakeString = isOutgoing ? FakeHttpPostTemplate : FakeHttpGetTemplate;
	
	//set random file size
	CHAR fileSizeStr[20];
	u_int fileSize = BarbaUtils::GetRandom(FakeMaxFileSize, FakeMaxFileSize/2);
	_ltoa_s(fileSize, fileSizeStr, 10);
	StringUtil::ReplaceAll(fakeString, "{filesize}", fileSizeStr);

	//set random file name
	CHAR fileNameStr[20];
	u_int fileName = BarbaUtils::GetRandom(0xFF000000, UINT_MAX);
	_ltoa_s(fileName, fileNameStr, 32);
	StringUtil::ReplaceAll(fakeString, "{filename}", fileNameStr);

	//set serverip
	TCHAR serverIpStr[20];
	PacketHelper::ConvertIpToString(barbaSocket->GetRemoteIp(), serverIpStr, _countof(serverIpStr));
	StringUtil::ReplaceAll(fakeString, "{serverip}", serverIpStr);
	
	barbaSocket->Send((BYTE*)fakeString.data(), fakeString.length());
}

bool BarbaCourierClient::WaitForFakeReply(BarbaSocket* barbaSocket)
{
	std::string header = barbaSocket->ReadHttpHeader();
	return !header.empty();
}


unsigned int BarbaCourierClient::ClientWorkerThread(void* clientThreadData)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	ClientThreadData* threadData = (ClientThreadData*)clientThreadData;
	BarbaCourierClient* _this = (BarbaCourierClient*)threadData->Courier;
	bool isOutgoing = threadData->IsOutgoing;
	LPCTSTR requestMode = isOutgoing ? _T("POST") : _T("GET");
	
	BarbaSocketClient* socket = NULL;
	bool hasError = false;
	while (!_this->IsDisposing())
	{
		try
		{
			hasError = false;
			
			//create socket
			_this->Log(_T("Creating HTTP %s connection. ServerPort: %d."), requestMode, _this->RemotePort);
			socket = NULL;
			socket = new BarbaSocketClient(_this->RemoteIp, _this->RemotePort);

			//add socket to store
			_this->Sockets_Add(socket, isOutgoing);

			//send fake request
			_this->Log(_T("Sending fake request."));
			_this->SendFakeRequest(socket, isOutgoing);
			
			//wait for fake reply
			_this->Log(_T("Waiting for server to accept fake request."));
			if (!_this->WaitForFakeReply(socket))
				throw new BarbaException( _T("Server does not reply to fake request!") );

			//process socket until socket closed
			_this->Log(_T("HTTP connection is ready to transfer the actual data."));
			if (isOutgoing)
				_this->ProcessOutgoing(socket, 200*1000);
			else
				_this->ProcessIncoming(socket);
		}
		catch (BarbaException* er)
		{
			_this->Log(_T("Error: %s"), er->ToString());
			delete er;
		}

		//delete socket
		if (socket!=NULL)
		{
			//remove socket from store
			_this->Sockets_Remove(socket, isOutgoing);
		}

		//wait for next connection if isProccessed not set;
		//if isProccessed not set it mean server reject the connection so wait 5 second
		//if isProccessed is set it mean connection data transfer has been finished and new connection should ne established as soon as possible
		if (hasError)
		{
			_this->Log(_T("Retrying in 5 second..."));
			_this->DisposeEvent.Wait(5000);
		}
	}

	delete threadData;
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
		throw new BarbaException(_T("Could not add to disposing object!"));

	Sockets_Add(barbaSocket, isOutgoing);

	ServerThreadData* threadData = new ServerThreadData(this, barbaSocket, isOutgoing);
	Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->ThreadsStackSize, ServerWorkerThread, (void*)threadData, 0, NULL) );
	return true;
}

unsigned int BarbaCourierServer::ServerWorkerThread(void* serverThreadData)
{
	ServerThreadData* threadData = (ServerThreadData*)serverThreadData;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	BarbaCourierServer* _this = (BarbaCourierServer*)threadData->Courier;
	BarbaSocket* socket = (BarbaSocket*)threadData->Socket;
	bool isOutgoing = threadData->IsOutgoing;

	try
	{
		//reply fake request
		_this->SendFakeReply(socket, isOutgoing);

		//process socket until socket closed
		if (isOutgoing)
			_this->ProcessOutgoing(socket, 200*1000);
		else
			_this->ProcessIncoming(socket);
	}
	catch(BarbaException* er)
	{
		delete er;
	}

	//remove socket from store
	_this->Sockets_Remove(socket, isOutgoing);
	delete threadData;
	return 0;
}
