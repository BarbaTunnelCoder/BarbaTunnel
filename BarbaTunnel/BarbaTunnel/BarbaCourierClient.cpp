#include "StdAfx.h"
#include "BarbaCourierClient.h"
#include "BarbaUtils.h"


BarbaCourierClient::BarbaCourierClient(BarbaCourierCreateStrcut* cs, DWORD remoteIp, u_short remotePort)
	: BarbaCourier(cs)
{
	this->RemoteIp = remoteIp;
	this->RemotePort = remotePort;

	for (u_short i=0; i<this->CreateStruct.MaxConnection; i++)
	{
		//create outgoing connection thread
		ClientThreadData* outgoingThreadData = new ClientThreadData(this, true);
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->CreateStruct.ThreadsStackSize, ClientWorkerThread, outgoingThreadData, 0, NULL));

		//create incoming connection thread
		ClientThreadData* incomingThreadData = new ClientThreadData(this, false);
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->CreateStruct.ThreadsStackSize, ClientWorkerThread, incomingThreadData, 0, NULL));
	}

	if (this->CreateStruct.KeepAliveInterval!=0)
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->CreateStruct.ThreadsStackSize, CheckKeepAliveThread, this, 0, NULL));
}

BarbaCourierClient::~BarbaCourierClient()
{
}

u_int BarbaCourierClient::SendFakeRequest(BarbaSocket* socket, std::vector<BYTE>* fakeFileHeader)
{
	bool outgoing = fakeFileHeader!=NULL;
	u_int fileSize;
	TCHAR filename[MAX_PATH];
	std::tstring contentType;
	GetFakeFile(filename, &contentType, &fileSize, fakeFileHeader, true);
	u_int fakeFileHeaderSize = fakeFileHeader!=NULL ? fakeFileHeader->size() : 0;
	//LPCTSTR requestMode = outgoing ? _T("POST") : _T("GET");

	//set serverip
	TCHAR serverIp[20];
	PacketHelper::ConvertIpToString(socket->GetRemoteIp(), serverIp, _countof(serverIp));

	std::tstring fakeRequest = outgoing ? this->CreateStruct.FakeHttpPostTemplate : this->CreateStruct.FakeHttpGetTemplate;
	InitFakeRequestVars(fakeRequest, filename, contentType.data(), fileSize, fakeFileHeaderSize);

	if (outgoing)
		Log(_T("Sending fake POST request! File: %s (%u KB)."), filename, fileSize/1000);
	else
		Log(_T("Sending fake HTTP GET request! FileName: %s."), filename);
	std::string fakeRequestA = fakeRequest;
	if (socket->Send((BYTE*)fakeRequestA.data(), fakeRequestA.length())!=(int)fakeRequestA.length())
		throw new BarbaException(_T("Could not send fake request!"));

	return fileSize;
}

void BarbaCourierClient::CheckKeepAlive()
{
	//close connection that does not receive keep alive
	if (this->CreateStruct.KeepAliveInterval==0 || this->LastSentTime==0)
		return; //keep live not enabled or no data sent yet

	SimpleSafeList<BarbaSocket*>* list = &this->IncomingSockets;
	SimpleSafeList<BarbaSocket*>::AutoLockBuffer autoLockBuf(list);
	BarbaSocket** sockets = autoLockBuf.GetBuffer();
	for (size_t i=0; i<list->GetCount(); i++)
	{
		if ( sockets[i]->GetLastReceivedTime() < this->LastSentTime &&
			GetTickCount() > (sockets[i]->GetLastReceivedTime() + this->CreateStruct.KeepAliveInterval*2) )
		{
			Log(_T("Connection closed due to keep alive timeout!"));
			sockets[i]->Close();
		}
	}
}

unsigned int __stdcall BarbaCourierClient::CheckKeepAliveThread(void* barbaCourier)
{
	BarbaCourierClient* _this = (BarbaCourierClient*)barbaCourier;
	while (_this->DisposeEvent.Wait(2000)==WAIT_TIMEOUT)
	{
		_this->CheckKeepAlive();
	}
	return 0;
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
			_this->Log(_T("HTTP %s connection added. Connections Count: %d."), requestMode, isOutgoing ? _this->OutgoingSockets.GetCount() : _this->IncomingSockets.GetCount());

			if (isOutgoing)
			{
				std::vector<BYTE> fakeFileHeader;
				u_int fakeFileSize = _this->SendFakeRequest(socket, &fakeFileHeader);

				//sending fake file header
				_this->SendFakeFileHeader(socket, &fakeFileHeader);

				//process socket until socket closed
				_this->ProcessOutgoing(socket, fakeFileSize - fakeFileHeader.size());

				//wait for fake reply
				_this->Log(_T("Waiting for server to accept fake HTTP POST request."));
				std::string header = socket->ReadHttpRequest();
				if (header.empty())
					throw new BarbaException( _T("Server does not reply to fake request!") );
			}
			else
			{
				//send GET fake request
				_this->SendFakeRequest(socket, NULL);

				//wait for fake reply
				_this->Log(_T("Waiting for server to accept fake HTTP GET request."));
				std::string httpReply = socket->ReadHttpRequest();
				if (httpReply.empty())
					throw new BarbaException( _T("Server does not reply to fake request!") );

				//wait for incoming fake file header
				_this->WaitForIncomingFakeHeader(socket, httpReply.data());

				//process socket until socket closed
				_this->ProcessIncoming(socket);
			}
		}
		catch (BarbaException* er)
		{
			hasError = true;
			_this->Log(_T("Error: %s"), er->ToString());
			delete er;
		}

		//delete socket
		if (socket!=NULL)
		{
			//remove socket from store
			_this->Sockets_Remove(socket, isOutgoing);
			_this->Log(_T("HTTP %s connection removed.  Connections Count: %d."), requestMode, isOutgoing ? _this->OutgoingSockets.GetCount() : _this->IncomingSockets.GetCount());
		}

		//wait for next connection if isProccessed not set;
		//if isProccessed not set it mean server reject the connection so wait 5 second
		//if isProccessed is set it mean connection data transfer has been finished and new connection should ne established as soon as possible
		if (hasError)
		{
			_this->Log(_T("Retrying in 10 second..."));
			_this->DisposeEvent.Wait(10000);
		}
	}

	delete threadData;
	return 0;
}

