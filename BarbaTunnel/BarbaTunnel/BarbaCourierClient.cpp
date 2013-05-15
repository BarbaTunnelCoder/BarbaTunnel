#include "StdAfx.h"
#include "BarbaCourierClient.h"
#include "BarbaUtils.h"


BarbaCourierClient::BarbaCourierClient(BarbaCourier::CreateStrcutBag* cs, DWORD remoteIp, u_short remotePort)
	: BarbaCourierHttp(cs)
{
	this->RemoteIp = remoteIp;
	this->RemotePort = remotePort;

	for (u_short i=0; i<this->CreateStruct.MaxConnection; i++)
	{
		//create outgoing connection thread (POST)
		ClientThreadData* outgoingThreadData = new ClientThreadData(this, true);
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->CreateStruct.ThreadsStackSize, ClientWorkerThread, outgoingThreadData, 0, NULL));

		//create incoming connection thread (GET)
		ClientThreadData* incomingThreadData = new ClientThreadData(this, false);
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->CreateStruct.ThreadsStackSize, ClientWorkerThread, incomingThreadData, 0, NULL));
	}

	if (this->CreateStruct.KeepAliveInterval!=0)
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->CreateStruct.ThreadsStackSize, CheckKeepAliveThread, this, 0, NULL));
}

BarbaCourierClient::~BarbaCourierClient()
{
}

void BarbaCourierClient::GetPostRequestBombard(size_t dataLength, BarbaBuffer* requestBuffer)
{
	TCHAR filename[MAX_PATH];
	std::tstring contentType;
	GetFakeFile(filename, &contentType, NULL, NULL, true);

	//use non bombard request for initialize request
	std::tstring request = GetHttpPostTemplate(true);
	InitRequestVars(request, filename, contentType.data(), dataLength, 0, true);

	//send request
	requestBuffer->append((char*)request.data(), request.size());
}

size_t BarbaCourierClient::SendPostRequest(BarbaSocket* socket, bool initBombard)
{
	size_t fileSize = 0;
	TCHAR filename[MAX_PATH];
	std::tstring contentType;
	BarbaBuffer fileHeader;

	//Prepare requests
	if (initBombard)
	{
		GetFakeFile(filename, &contentType, &fileSize, NULL, true);
	}
	else
	{
		GetFakeFile(filename, &contentType, &fileSize, &fileHeader, true);
		Log2(_T("Sending HTTP POST request! FileName: %s, FileSize: %d KB."), filename, fileSize/1000);
	}


	//use non bombard request for initialize request
	std::tstring request = GetHttpPostTemplate(false);
	InitRequestVars(request, filename, contentType.data(), fileSize, fileHeader.size(), true);

	//send request
	if (socket->Send((BYTE*)request.data(), request.length())!=(int)request.length())
		throw new BarbaException(_T("Could not send POST request!"));

	//sending file header in no bombard mode
	if (fileHeader.size()!=0)
	{
		Log2(_T("Sending File Header! HeaderSize: %d KB"), fileHeader.size()/1000);
		SendFileHeader(socket, &fileHeader);
	}

	return fileSize - fileHeader.size();

}

void BarbaCourierClient::WaitForAcceptPostRequest(BarbaSocket* barbaSocket)
{
	DWORD oldTimeOut = barbaSocket->GetReceiveTimeOut();
	try
	{
		barbaSocket->SetReceiveTimeOut(10000);
		std::string header = barbaSocket->ReadHttpRequest();
		if (header.empty())
			throw new BarbaException( _T("Server does not accept request!") );
		barbaSocket->SetReceiveTimeOut(oldTimeOut);

		//process PostReplyPayload
		size_t contentLength = BarbaUtils::GetKeyValueFromString(header.data(), _T("Content-Length"), 0);
		if (contentLength>0)
		{
			ProcessIncomingMessage(barbaSocket, 0, contentLength);
			Log3("Receiving from POST-REPLY payload. Count: %d", contentLength);
		}
	}
	catch (...)
	{
		barbaSocket->SetReceiveTimeOut(oldTimeOut);
		throw;
	}
}


size_t BarbaCourierClient::SendGetRequest(BarbaSocket* socket)
{
	TCHAR filename[MAX_PATH];

	size_t fileSize = 0;
	std::tstring contentType;
	GetFakeFile(filename, &contentType, &fileSize, NULL, true);

	std::tstring request = GetHttpGetTemplate(false);
	InitRequestVars(request, filename, NULL, fileSize, 0, false);

	//Log
	if (!IsBombardGet)
		Log2(_T("Sending HTTP GET request! FileName: %s."), filename);

	//send request
	if (socket->Send((BYTE*)request.data(), request.length())!=(int)request.length())
		throw new BarbaException(_T("Could not send GET request!"));

	return fileSize;
}

void BarbaCourierClient::SendGetRequestBombard(BarbaSocket* socket, BarbaBuffer* content)
{
	TCHAR filename[MAX_PATH];

	std::tstring contentType;
	GetFakeFile(filename, &contentType, NULL, NULL, true);

	std::tstring request = GetHttpGetTemplate(true);
	InitRequestVars(request, filename, NULL, content->size(), 0, false);

	//send buffer
	BarbaBuffer buffer;
	buffer.reserve(request.length() + content->size());
	buffer.append((char*)request.data(), request.size());
	buffer.append(content);
	if (content->size()>0)
		Log3(_T("Sending with GET payload. Count: %d"), content->size());

	//send request
	if (socket->Send(buffer.data(), buffer.size())!=(int)buffer.size())
		throw new BarbaException(_T("Could not send GET request!"));

}

void BarbaCourierClient::BeforeSendMessage(BarbaSocket* /*barbaSocket*/, BarbaBuffer* messageBuffer)
{
	if (IsBombardPost)
	{
		BarbaBuffer buffer;
		GetPostRequestBombard(messageBuffer->size(), &buffer);
		buffer.append(messageBuffer);
		messageBuffer->assign(&buffer);
		Log3(_T("Sending with POST request, Count: %d"), messageBuffer->size());
	}
	else
	{
		Log3(_T("Sending to POST channel, Count: %d"), messageBuffer->size());
	}
}

void BarbaCourierClient::AfterSendMessage(BarbaSocket* barbaSocket, bool isTransferFinished)
{
	if (IsBombardPostReply && !isTransferFinished)
	{
		WaitForAcceptPostRequest(barbaSocket);
	}
}

void BarbaCourierClient::BeforeReceiveMessage(BarbaSocket* barbaSocket, size_t* chunkSize)
{
	if (IsBombardGet)
	{
		std::string header = barbaSocket->ReadHttpRequest();
		if (header.empty())
			throw new BarbaException( _T("Server does not accept request!") );
		*chunkSize = BarbaUtils::GetKeyValueFromString(header.data(), _T("Content-Length"), 0);
		if (*chunkSize>0)
			Log3("Receiving from GET request. Count: %d", *chunkSize);
	}
}


void BarbaCourierClient::AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength, bool isTransferFinished)
{
	if (IsBombardGet && !isTransferFinished)
	{

		BarbaArray<Message*> messages;
		try
		{
			//attach some message to get request
			BarbaBuffer buffer;
			if (IsBombardGetPayload)
			{
				GetMessages(messages);
				ProcessOutgoingMessages(messages, 0, 0, &buffer);
				if (buffer.size()>0)
					Log3("Sending with GET Payload. Count: %d", buffer.size());
			}

			//send request
			SendGetRequestBombard(barbaSocket, &buffer);

			//delete sent message
			for (size_t i=0; i<messages.size(); i++)
				delete messages[i];
			messages.clear();
		}
		catch (...)
		{
			Send(messages, true);
			throw;
		}
	}
	else
	{
		Log3("Receiving from GET channel. Count: %d", messageLength);
	}
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
			Log2(_T("Connection closed due to keep alive timeout!"));
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
	DWORD retryTime = 10000;

	while (!_this->IsDisposing())
	{
		try
		{
			hasError = false;

			//create socket
			socket = NULL;
			socket = new BarbaSocketClient(_this->RemoteIp, _this->RemotePort);

			//add socket to store
			_this->Sockets_Add(socket, isOutgoing);

			if (isOutgoing)
			{
				if (_this->IsBombardPost || _this->IsBombardPostReply)
				{
					//report new connection
					_this->Log2(_T("HTTP Bombard %s added. Connections Count: %d."), _this->IsBombardPostReply ? _T("POST & REPLY") : _T("POST"), _this->OutgoingSockets.GetCount());
					retryTime = 3000;

					//send initial post request
					size_t fileSize = _this->SendPostRequest(socket, true);

					//Wait to accept initial bombard post request
					_this->Log2(_T("Waiting for server to accept HTTP POST request."));
					_this->WaitForAcceptPostRequest(socket);

					//process socket until socket closed
					_this->ProcessOutgoing(socket, fileSize);

					//finish TCP connection
					_this->Log2(_T("HTTP Bombard POST transfer completed."));

				}
				else
				{
					//report new connection
					_this->Log2(_T("HTTP POST added. Connections Count: %d."), _this->OutgoingSockets.GetCount());
					size_t fileSize = _this->SendPostRequest(socket, false);

					//process socket until socket closed or finish uploading fakeFileSize
					_this->ProcessOutgoing(socket, fileSize);

					//wait for accept post request
					_this->WaitForAcceptPostRequest(socket);
					_this->Log2(_T("HTTP POST file completed."));
				}
			}
			else
			{
				_this->Log2(_T("HTTP %s added. Connections Count: %d."), _this->IsBombardGet ? _T("Bombard GET") : _T("GET"), _this->IncomingSockets.GetCount());

				//send GET request
				size_t fileSize = _this->SendGetRequest(socket);

				if (_this->IsBombardGet)
				{
					retryTime = 3000;

					//process socket until socket closed
					_this->ProcessIncoming(socket, fileSize);

					//finish current connection
					_this->Log2(_T("HTTP Bombard GET Transfer Completed."));
				}
				else
				{
					//wait for reply
					_this->Log2(_T("Waiting for server to accept HTTP GET request."));
					std::string httpReply = socket->ReadHttpRequest();
					if (httpReply.empty())
						throw new BarbaException( _T("Server does not reply to HTTP GET request!") );

					//process reply
					std::tstring requestData = _this->GetRequestDataFromHttpRequest(httpReply.data());
					size_t fileHeaderSize = BarbaUtils::GetKeyValueFromString(requestData.data(), _T("fileHeaderSize"), 0);

					//wait for incoming file header
					_this->Log2(_T("Downloading file %u KB. HeaderSize: %u KB"), fileSize/1000, fileHeaderSize/1000);
					_this->WaitForIncomingFileHeader(socket, fileHeaderSize);

					//process socket until socket closed
					_this->ProcessIncoming(socket, fileSize-fileHeaderSize);

					//report
					_this->Log2(_T("HTTP GET file completed."));
				}
			}
		}
		catch (BarbaException* er)
		{
			hasError = true;
			_this->Log2(_T("Error: %s"), er->ToString());
			delete er;
		}

		//delete socket
		if (socket!=NULL)
		{
			//remove socket from store
			_this->Sockets_Remove(socket, isOutgoing);
			_this->Log2(_T("HTTP %s connection removed.  Connections Count: %d."), requestMode, isOutgoing ? _this->OutgoingSockets.GetCount() : _this->IncomingSockets.GetCount());
		}

		//wait for next connection if isProccessed not set;
		//if isProccessed not set it mean server reject the connection so wait 5 second
		//if isProccessed is set it mean connection data transfer has been finished and new connection should ne established as soon as possible
		if (hasError)
		{
			_this->Log2(_T("Retrying in %d second..."), retryTime/1000);
			_this->DisposeEvent.Wait(retryTime);
		}
	}

	delete threadData;
	return 0;
}

