#include "StdAfx.h"
#include "BarbaCourierServer.h"
#include "BarbaUtils.h"

BarbaCourierServer::BarbaCourierServer(BarbaCourierCreateStrcut* cs)
	: BarbaCourier(cs)
{
}


BarbaCourierServer::~BarbaCourierServer(void)
{
}

bool BarbaCourierServer::AddSocket(BarbaSocket* barbaSocket, LPCSTR httpRequest, bool isOutgoing)
{
	if (this->IsDisposing())
		throw new BarbaException(_T("Could not add to disposing object!"));

	Sockets_Add(barbaSocket, isOutgoing);
	std::tstring requestData = GetRequestDataFromHttpRequest(httpRequest);

	//set fakePacketMinSize
	if (this->CreateStruct.FakePacketMinSize==0) //can change only one time; instead using mutex
		this->CreateStruct.FakePacketMinSize = (u_short)BarbaUtils::GetKeyValueFromString(requestData.data(), _T("packetminsize"), 0);

	//set KeepAliveInterval
	if (this->CreateStruct.KeepAliveInterval==0) //can change only one time; instead using mutex
		this->CreateStruct.KeepAliveInterval = (u_short)BarbaUtils::GetKeyValueFromString(requestData.data(), _T("keepalive"), 0);

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
			std::vector<BYTE> fakeFileHeader;
			u_int remainBytes = _this->SendFakeReply(socket, threadData->HttpRequest.data(), &fakeFileHeader);

			//sending fake file header
			_this->SendFakeFileHeader(socket, &fakeFileHeader);
		
			//process socket until socket closed
			_this->ProcessOutgoing(socket, remainBytes);
		}
		else
		{
			//wait for incoming fake file header
			_this->WaitForIncomingFakeHeader(socket, threadData->HttpRequest.data());

			//process socket until socket closed
			_this->ProcessIncoming(socket);

			//send fake reply 
			_this->SendFakeReply(socket, threadData->HttpRequest.data(), NULL);
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

u_int BarbaCourierServer::SendFakeReply(BarbaSocket* socket, LPCTSTR httpRequest, std::vector<BYTE>* fakeFileHeader)
{
	bool outgoing = fakeFileHeader!=NULL;
	std::tstring fakeUrl = BarbaUtils::GetFileUrlFromHttpRequest(httpRequest);
	std::tstring fakefile = BarbaUtils::GetFileNameFromUrl(fakeUrl.data());
	
	u_int fileSize;
	TCHAR filename[MAX_PATH];
	_tcscpy_s(filename, fakefile.data());
	std::tstring contentType;
	GetFakeFile(filename, &contentType, &fileSize, fakeFileHeader, false);
	u_int fakeFileHeaderSize = fakeFileHeader!=NULL ? fakeFileHeader->size() : 0;

	std::tstring fakeReply = outgoing ? this->CreateStruct.FakeHttpGetTemplate : this->CreateStruct.FakeHttpPostTemplate;
	InitFakeRequestVars(fakeReply, filename, contentType.data(), fileSize, fakeFileHeaderSize);

	if (outgoing)
		Log(_T("Sending fake GET reply! File: %s (%u KB)."), filename, fileSize, fakeFileHeaderSize);
	else
		Log(_T("Sending fake POST reply! FileName: %s."), filename);

	std::string fakeReplyA = fakeReply;
	if (socket->Send((BYTE*)fakeReplyA.data(), fakeReplyA.size())!=(int)fakeReplyA.size())
		throw new BarbaException(_T("Could not send fake reply."));

	return fileSize;
}
