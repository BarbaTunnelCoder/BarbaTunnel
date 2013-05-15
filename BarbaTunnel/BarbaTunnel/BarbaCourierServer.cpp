#include "StdAfx.h"
#include "BarbaCourierServer.h"
#include "BarbaUtils.h"

BarbaCourierServer::BarbaCourierServer(BarbaCourier::CreateStrcutBag* cs)
	: BarbaCourierHttp(cs)
{
}


BarbaCourierServer::~BarbaCourierServer(void)
{
}

void BarbaCourierServer::Init(LPCTSTR requestData)
{
	//set fakePacketMinSize
	this->CreateStruct.FakePacketMinSize = (u_short)BarbaUtils::GetKeyValueFromString(requestData, _T("packetMinSize"), 0);

	//set fakePacketMinSize
	this->CreateStruct.HttpBombardMode = BarbaUtils::GetKeyValueFromString(requestData, _T("bombardMode"));
	if (!this->CreateStruct.AllowBombard && !this->CreateStruct.HttpBombardMode.empty())
		throw new BarbaException(_T("RequestBombard does not allowed by server!"));

	//set KeepAliveInterval
	this->CreateStruct.KeepAliveInterval = (u_short)BarbaUtils::GetKeyValueFromString(requestData, _T("keepalive"), 0);

	//Validate Paramters
	RefreshParameters(); //Warning: Calling virtual function in constructor will not call overrided one 
}

bool BarbaCourierServer::AddSocket(BarbaSocket* barbaSocket, LPCSTR httpRequest, LPCTSTR requestData, bool isOutgoing)
{
	if (this->IsDisposing())
		throw new BarbaException(_T("Could not add to disposing object!"));

	//Add Socket
	Sockets_Add(barbaSocket, isOutgoing);

	//start threads
	ServerThreadData* threadData = new ServerThreadData(this, barbaSocket, httpRequest, requestData, isOutgoing);
	Threads.AddTail( (HANDLE)_beginthreadex(NULL, this->CreateStruct.ThreadsStackSize, ServerWorkerThread, (void*)threadData, 0, NULL) );
	return true;
}

unsigned int BarbaCourierServer::ServerWorkerThread(void* serverThreadData)
{
	ServerThreadData* threadData = (ServerThreadData*)serverThreadData;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	BarbaCourierServer* _this = (BarbaCourierServer*)threadData->Courier;
	BarbaSocket* socket = (BarbaSocket*)threadData->Socket;
	bool isOutgoing = threadData->IsOutgoing;
	size_t transferSize = BarbaUtils::GetKeyValueFromString(threadData->RequestData.data(), _T("transferSize"), 0);
	
	try
	{
		if (isOutgoing)
		{
			if (_this->IsBombardGet)
			{
				//report new connection
				_this->Log2(_T("HTTP Bombard GET added. Connections Count: %d."), _this->OutgoingSockets.GetCount());

				//process socket until filesize transfered
				_this->ProcessOutgoing(socket, transferSize);

				//finish current connection
				_this->Log2(_T("HTTP Bombard GET Transfer Completed."));
			}
			else
			{
				//report new connection
				_this->Log2(_T("HTTP GET added. Connections Count: %d."), _this->OutgoingSockets.GetCount());

				//send file header
				BarbaBuffer fileHeader;
				std::tstring requestUrl = BarbaUtils::GetFileUrlFromHttpRequest(threadData->HttpRequest.data());
				size_t remainBytes = _this->SendGetReply(socket, requestUrl.data(), transferSize, &fileHeader);

				//sending fake file header
				_this->SendFileHeader(socket, &fileHeader);
				remainBytes -= fileHeader.size();

				//process socket until socket closed or or file transfered
				_this->ProcessOutgoing(socket, remainBytes);

				//report
				_this->Log2(_T("File Sent."));
			}
		}
		else
		{
			if (_this->IsBombardPost || _this->IsBombardPostReply)
			{
				//report new connection
				_this->Log2(_T("HTTP Bombard %s added. Connections Count: %d."), _this->IsBombardPostReply ? _T("POST & REPLY") : _T("POST") , _this->IncomingSockets.GetCount());

				//reply to initial request. In initial send always even if IsBombardPostReply is false
				_this->SendPostReply(socket);

				//process socket until socket closed
				_this->ProcessIncoming(socket, transferSize);

				//finish current connection
				_this->Log2(_T("HTTP Bombard POST Transfer Completed."));
			}
			else
			{
				//report new connection
				_this->Log2(_T("HTTP POST added. Connections Count: %d."), _this->IncomingSockets.GetCount());

				//Get Initial data
				size_t fileHeaderSize = BarbaUtils::GetKeyValueFromString(threadData->RequestData.data(), _T("fileHeaderSize"), 0);

				//wait for incoming fake file header
				_this->Log2(_T("Receiving file %u KB."), transferSize/1000);
				_this->WaitForIncomingFileHeader(socket, fileHeaderSize);

				//process socket until socket closed or file transfered
				_this->ProcessIncoming(socket, transferSize-fileHeaderSize);

				//Send POST Reply that file downloaded
				_this->Log2(_T("Sending POST reply!"));
				_this->SendPostReply(socket);

				//File received.
				_this->Log2(_T("File received."));
			}
		}
	}
	catch(BarbaException* er)
	{
		_this->Log2(_T("Error: %s"), er->ToString());
		delete er;
	}
	catch(...)
	{
		_this->Log2(_T("Unknown Error!"));
	}

	//remove socket from store
	_this->Sockets_Remove(socket, isOutgoing);
	_this->Log2(_T("HTTP %s connection removed. Connections Count: %d."), isOutgoing ? _T("GET") : _T("POST"), isOutgoing ? _this->OutgoingSockets.GetCount() : _this->IncomingSockets.GetCount());
	delete threadData;
	return 0;
}

void BarbaCourierServer::BeforeSendMessage(BarbaSocket* /*barbaSocket*/, BarbaBuffer* messageBuffer) 
{
	if (IsBombardGet)
	{
		BarbaBuffer buffer;
		GetGetReplyBombard(messageBuffer->size(), &buffer);
		buffer.append(messageBuffer);
		messageBuffer->assign(&buffer);
		Log3(_T("Sending with GET request. Count: %d"), messageBuffer->size());
	}
	else
	{
		Log3(_T("Sending to GET channel. Count: %d"), messageBuffer->size());
	}
}

void BarbaCourierServer::AfterSendMessage(BarbaSocket* barbaSocket, bool isTransferFinished)
{
	//waiting for another get request in GET bombard mode
	if (IsBombardGet && !isTransferFinished)
	{
		std::string header = barbaSocket->ReadHttpRequest();
		size_t contentLength = BarbaUtils::GetKeyValueFromString(header.data(), _T("Content-Length"), 0);
		if (contentLength>0)
		{
			ProcessIncomingMessage(barbaSocket, 0, contentLength);
			Log3(_T("Receiving from GET request payload: Count: %d"), contentLength);
		}
	}
}

void BarbaCourierServer::BeforeReceiveMessage(BarbaSocket* barbaSocket, size_t* chunkSize)
{
	if (IsBombardPost)
	{
		std::string header = barbaSocket->ReadHttpRequest();
		if (header.empty())
			throw new BarbaException( _T("Server does not accept request!") );
		*chunkSize = BarbaUtils::GetKeyValueFromString(header.data(), _T("Content-Length"), 0);
		Log3(_T("Receiving from POST request: Count: %d"), *chunkSize);
	}
}

void BarbaCourierServer::AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength, bool isTransferFinished)
{
	if (IsBombardPostReply && !isTransferFinished)
	{
		BarbaArray<Message*> messages;
		try
		{
			//attach some message to get request
			BarbaBuffer buffer;
			if (IsBombardPostReplyPayload)
			{
				GetMessages(messages);
				ProcessOutgoingMessages(messages, 0, 0, &buffer);
				if (buffer.size()>0) 
					Log3("Sending with POST-REPLY payload. Count: %d", buffer.size());
			}
			else
			{
				Log3("Receiving with POST request. Count: %d", messageLength);
			}

			//send reply
			SendPostReply(barbaSocket, &buffer);

			//delete sent message
			for (int i=0; i<(int)messages.size(); i++)
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
		Log3("Receiving from POST channel. Count: %d", messageLength);
	}
}

void BarbaCourierServer::SendPostReply(BarbaSocket* socket)
{
	BarbaBuffer content;
	SendPostReply(socket, &content);
}

void BarbaCourierServer::SendPostReply(BarbaSocket* socket, BarbaBuffer* content)
{
	if ( IsDisposing() ) 
		throw new BarbaException(_T("Could not send request while disposing!"));

	//process replyPayload
	std::tstring postReply = GetHttpPostReplyRequest(IsBombardPost);
	InitRequestVars(postReply, NULL, NULL, content->size(), 0, true);

	//send buffer
	BarbaBuffer buffer;
	buffer.reserve(postReply.length() + content->size());
	buffer.append((char*)postReply.data(), postReply.size());
	buffer.append(content);

	if (socket->Send(buffer.data(), buffer.size())!=(int)buffer.size())
		throw new BarbaException(_T("Could not send post reply."));
}

size_t BarbaCourierServer::SendGetReply(BarbaSocket* socket, LPCTSTR fileUrl, size_t fileSize, BarbaBuffer* fileHeader)
{
	if ( IsDisposing() ) throw new BarbaException(_T("Could not send request while disposing!"));
	std::tstring requestFile = BarbaUtils::GetFileNameFromUrl(fileUrl);

	TCHAR filename[MAX_PATH];
	_tcscpy_s(filename, requestFile .data());
	std::tstring contentType;
	GetFakeFile(filename, &contentType, NULL, fileHeader, false);
	size_t fakeFileHeaderSize = fileHeader!=NULL ? fileHeader->size() : 0;

	std::tstring reply = GetHttpGetReplyRequest(false);
	InitRequestVars(reply, filename, contentType.data(), fileSize, fakeFileHeaderSize, true);

	Log2(_T("Sending GET reply! File: %s (%u KB)."), filename, fileSize/1000, fakeFileHeaderSize);
	std::string replyA = replyA;
	if (socket->Send((BYTE*)reply.data(), reply.size())!=(int)reply.size())
		throw new BarbaException(_T("Could not send GET reply."));

	return fileSize;
}

void BarbaCourierServer::GetGetReplyBombard(size_t dataLength, BarbaBuffer* requestBuffer)
{
	std::tstring reply = GetHttpGetReplyRequest(true);
	InitRequestVars(reply, NULL, NULL, dataLength, 0, true);
	requestBuffer->append((char*)reply.data(), reply.size());
}

