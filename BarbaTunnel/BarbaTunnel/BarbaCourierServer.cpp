#include "StdAfx.h"
#include "BarbaCourierServer.h"
#include "BarbaUtils.h"

BarbaCourierServer::BarbaCourierServer(BarbaCourier::CreateStrcutBag* cs)
	: BarbaCourier(cs)
{
}


BarbaCourierServer::~BarbaCourierServer(void)
{
}

void BarbaCourierServer::Init(LPCTSTR requestData)
{
	//set fakePacketMinSize
	this->CreateStruct.FakePacketMinSize = (u_short)BarbaUtils::GetKeyValueFromString(requestData, _T("packetminsize"), 0);

	//set fakePacketMinSize
	this->CreateStruct.HttpBombardMode = BarbaUtils::GetKeyValueFromString(requestData, _T("bombardmode"));
	if (!this->CreateStruct.AllowBombard && !this->CreateStruct.HttpBombardMode.empty())
		throw new BarbaException(_T("RequestPerPacket does not allowed by server!"));

	//set KeepAliveInterval
	this->CreateStruct.KeepAliveInterval = (u_short)BarbaUtils::GetKeyValueFromString(requestData, _T("keepalive"), 0);

	//Validate Paramters
	RefreshParameters();
}

bool BarbaCourierServer::AddSocket(BarbaSocket* barbaSocket, LPCSTR httpRequest, bool isOutgoing)
{
	if (this->IsDisposing())
		throw new BarbaException(_T("Could not add to disposing object!"));


	//Add Socket
	Sockets_Add(barbaSocket, isOutgoing);

	//start threads
	ServerThreadData* threadData = new ServerThreadData(this, barbaSocket, httpRequest, isOutgoing);
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
	LPCTSTR requestMode = isOutgoing ? _T("GET") : _T("POST"); //request always come from client
	
	//report new connection in current thread
	_this->Log(_T("HTTP %s connection added. Connections Count: %d."), requestMode,  isOutgoing ? _this->OutgoingSockets.GetCount() : _this->IncomingSockets.GetCount());

	try
	{
		if (isOutgoing)
		{
			//send fake reply 
			BarbaBuffer fileHeader;
			size_t remainBytes = _this->SendGetReply(socket, threadData->HttpRequest.data(), &fileHeader);

			//sending fake file header
			_this->SendFileHeader(socket, &fileHeader);
			remainBytes -= fileHeader.size();
		
			//process socket until socket closed
			_this->ProcessOutgoing(socket, remainBytes);

			//report
			_this->Log(_T("Finish Sending file."));
		}
		else
		{
			std::tstring requestData = _this->GetRequestDataFromHttpRequest(threadData->HttpRequest.data());
			size_t fileLen = BarbaUtils::GetKeyValueFromString(requestData.data(), _T("filesize"), 0);
			size_t fileHeaderLen = BarbaUtils::GetKeyValueFromString(requestData.data(), _T("fileheadersize"), 0);;

			if (_this->IsBombardPost || _this->IsBombardPostReply)
			{
				//reply to initial request
				_this->SendPostReply(socket);
			}
			else
			{
				//wait for incoming fake file header
				_this->Log(_T("Receiving file %u KB."), fileLen/1000);
				_this->WaitForIncomingFileHeader(socket, fileHeaderLen);
			}

			//process socket until socket closed
			_this->ProcessIncoming(socket, fileLen-fileHeaderLen);

			//send fake reply 
			if (!_this->IsBombardPostReply)
			{
				_this->Log(_T("Sending POST reply! FileName: %s."));
				_this->SendPostReply(socket);
				_this->Log(_T("Finish Receiving file."));
			}
		}
	}
	catch(BarbaException* er)
	{
		_this->Log(_T("Error: %s"), er->ToString());
		delete er;
	}
	catch(...)
	{
		_this->Log(_T("Unknown Error!"));
	}

	//remove socket from store
	_this->Sockets_Remove(socket, isOutgoing);
	_this->Log(_T("HTTP %s connection removed. Connections Count: %d."), isOutgoing ? _T("GET") : _T("POST"), isOutgoing ? _this->OutgoingSockets.GetCount() : _this->IncomingSockets.GetCount());
	delete threadData;
	return 0;
}

void BarbaCourierServer::BeforeReceiveMessage(BarbaSocket* barbaSocket)
{
	if (!IsBombardPost)
		return;

	std::string ss = barbaSocket->ReadHttpRequest();
}

void BarbaCourierServer::AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength)
{
	Log("***** Message Received %u Byte", messageLength);
	if (!IsBombardPostReply)
		return;

	SendPostReply(barbaSocket);
}

void BarbaCourierServer::SendPostReply(BarbaSocket* socket)
{
	if ( IsDisposing() ) 
		throw new BarbaException(_T("Could not send request while disposing!"));

	std::tstring postReply = GetHttpPostReplyRequest(IsBombardPost);
	InitRequestVars(postReply, NULL, NULL, 0, 0);
	std::string postReplyA = postReply;
	if (socket->Send((BYTE*)postReplyA.data(), postReplyA.size())!=(int)postReplyA.size())
		throw new BarbaException(_T("Could not send post reply."));
}

size_t BarbaCourierServer::SendGetReply(BarbaSocket* socket, LPCTSTR httpRequest, BarbaBuffer* fileHeader)
{
	if ( IsDisposing() ) throw new BarbaException(_T("Could not send request while disposing!"));
	std::tstring requestUrl = BarbaUtils::GetFileUrlFromHttpRequest(httpRequest);
	std::tstring requestFile = BarbaUtils::GetFileNameFromUrl(requestUrl.data());
	
	size_t fileSize;
	TCHAR filename[MAX_PATH];
	_tcscpy_s(filename, requestFile .data());
	std::tstring contentType;
	GetFakeFile(filename, &contentType, &fileSize, fileHeader, false);
	size_t fakeFileHeaderSize = fileHeader!=NULL ? fileHeader->size() : 0;

	std::tstring fakeReply = GetHttpGetReplyRequest(false);
	InitRequestVars(fakeReply, filename, contentType.data(), fileSize, fakeFileHeaderSize);

	Log(_T("Sending GET reply! File: %s (%u KB)."), filename, fileSize/1000, fakeFileHeaderSize);
	std::string replyA = replyA;
	if (socket->Send((BYTE*)replyA.data(), replyA.size())!=(int)replyA.size())
		throw new BarbaException(_T("Could not send GET reply."));

	return fileSize;
}
