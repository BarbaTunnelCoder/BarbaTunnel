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

size_t BarbaCourierClient::SendPostRequestBombard(BarbaSocket* socket, size_t dataLength)
{
	TCHAR filename[MAX_PATH];
	std::tstring contentType;
	GetFakeFile(filename, &contentType, NULL, NULL, true);

	//use non bombard request for initialize request
	std::tstring fakeRequest = GetFakeRequest(true, true);
	InitRequestVars(fakeRequest, filename, contentType.data(), dataLength, 0);

	//send request
	std::string fakeRequestA = fakeRequest;
	if (socket->Send((BYTE*)fakeRequestA.data(), fakeRequestA.length())!=(int)fakeRequestA.length())
		throw new BarbaException(_T("Could not send fake request!"));

	return fakeRequestA.size();
}

size_t BarbaCourierClient::SendPostRequest(BarbaSocket* socket)
{
	size_t fileSize = 0;
	TCHAR filename[MAX_PATH];
	std::tstring contentType;
	BarbaBuffer fakeFileHeader;

	//Prepare requests
	LPCTSTR bombardInfo = NULL;
	if (IsBombardPost) bombardInfo = _T("Bombard POST");
	if (IsBombardPostReply) bombardInfo = _T("Bombard POST & REPLY");
	if (bombardInfo!=NULL)
	{
		GetFakeFile(filename, &contentType, NULL, NULL, true);
		Log(_T("Sending fake HTTP POST request! FileName: %s."), bombardInfo);
	}
	else
	{
		GetFakeFile(filename, &contentType, &fileSize, &fakeFileHeader, true);
		Log(_T("Sending fake HTTP POST request! %s!"), filename);
	}


	//use non bombard request for initialize request
	std::tstring fakeRequest = GetFakeRequest(true, false);
	InitRequestVars(fakeRequest, filename, contentType.data(), fileSize, fakeFileHeader.size());

	//send request
	std::string fakeRequestA = fakeRequest;
	if (socket->Send((BYTE*)fakeRequestA.data(), fakeRequestA.length())!=(int)fakeRequestA.length())
		throw new BarbaException(_T("Could not send fake request!"));

	//sending fake file header in no bombard mode
	if (fakeFileHeader.size()!=0)
		SendFakeFileHeader(socket, &fakeFileHeader);

	return fileSize - fakeFileHeader.size();

}

void BarbaCourierClient::WaitForAcceptPostRequest(BarbaSocket* socket)
{
	DWORD oldTimeOut = socket->GetReceiveTimeOut();
	try
	{
		socket->SetReceiveTimeOut(10000);
		std::string header = socket->ReadHttpRequest();
		if (header.empty())
			throw new BarbaException( _T("Server does not accept request!") );
		socket->SetReceiveTimeOut(oldTimeOut);
	}
	catch (...)
	{
		socket->SetReceiveTimeOut(oldTimeOut);
		throw;
	}
}


void BarbaCourierClient::SendGetRequest(BarbaSocket* socket)
{
	TCHAR filename[MAX_PATH];

	std::tstring contentType;
	GetFakeFile(filename, &contentType, NULL, NULL, true);

	std::tstring fakeRequest = GetFakeRequest(false, false);
	InitRequestVars(fakeRequest, filename, NULL, 0, 0);

	LPCTSTR bombardInfo = NULL;
	if (IsBombardGet) bombardInfo = _T("Bombard GET");
	if (bombardInfo!=NULL)
		Log(_T("Sending fake HTTP GET request! %s!"), bombardInfo);
	else
		Log(_T("Sending fake HTTP GET request! FileName: %s."), filename);

	//send request
	std::string fakeRequestA = fakeRequest;
	if (socket->Send((BYTE*)fakeRequestA.data(), fakeRequestA.length())!=(int)fakeRequestA.length())
		throw new BarbaException(_T("Could not send GET request!"));
}

void BarbaCourierClient::SendGetRequestBombard(BarbaSocket* socket)
{
	TCHAR filename[MAX_PATH];

	std::tstring contentType;
	GetFakeFile(filename, &contentType, NULL, NULL, true);

	std::tstring fakeRequest = GetFakeRequest(false, true);
	InitRequestVars(fakeRequest, filename, NULL, 0, 0);

	//send request
	std::string fakeRequestA = fakeRequest;
	if (socket->Send((BYTE*)fakeRequestA.data(), fakeRequestA.length())!=(int)fakeRequestA.length())
		throw new BarbaException(_T("Could not send GET request!"));
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
		if (sockets[i]->IsOpen() &&
			sockets[i]->GetLastReceivedTime() < this->LastSentTime &&
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

			//generate log for new connection
			_this->Log(_T("Creating HTTP %s connection. ServerPort: %d."), requestMode, _this->RemotePort);

			//create socket
			socket = NULL;
			socket = new BarbaSocketClient(_this->RemoteIp, _this->RemotePort);

			//add socket to store
			_this->Sockets_Add(socket, isOutgoing);
			_this->Log(_T("HTTP %s connection added. Connections Count: %d."), requestMode, isOutgoing ? _this->OutgoingSockets.GetCount() : _this->IncomingSockets.GetCount());

			if (isOutgoing)
			{
				size_t fakeFileSize = _this->SendPostRequest(socket);
				if (fakeFileSize==0)
				{
					_this->Log(_T("Waiting for server to accept fake HTTP POST request."));
					_this->WaitForAcceptPostRequest(socket);
				}

				//process socket until socket closed or finish uploading fakeFileSize
				_this->ProcessOutgoing(socket, fakeFileSize);

				//wait for fake reply in non bombard mode, in post bomard mode ProcessOutgoing is responsibe to wait for fake reply for each packet
				if (fakeFileSize!=0)
				{
					_this->WaitForAcceptPostRequest(socket);
					_this->Log(_T("Finish uploading file."));
				}
			}
			else
			{
				//send GET fake request
				_this->SendGetRequest(socket);

				//wait for fake reply
				_this->Log(_T("Waiting for server to accept HTTP GET request."));
				std::string httpReply = socket->ReadHttpRequest();
				if (httpReply.empty())
					throw new BarbaException( _T("Server does not reply to HTTP GET request!") );
				std::tstring requestData = _this->GetRequestDataFromHttpRequest(httpReply.data());
				size_t fileLen = BarbaUtils::GetKeyValueFromString(requestData.data(), _T("filesize"), 0);
				size_t fileHeaderLen = BarbaUtils::GetKeyValueFromString(requestData.data(), _T("fileheadersize"), 0);
				if (fileLen==0)
					throw new BarbaException( _T("Server replied zero file size! Make sure no other services on server is listening to tunnel port.") );

				//wait for incoming fake file header
				_this->Log(_T("Downloading file %u KB."), fileLen/1000);
				_this->WaitForIncomingFakeHeader(socket, fileHeaderLen);

				//process socket until socket closed
				_this->ProcessIncoming(socket, fileLen-fileHeaderLen);

				//report
				_this->Log(_T("Finish downloading file."));
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

