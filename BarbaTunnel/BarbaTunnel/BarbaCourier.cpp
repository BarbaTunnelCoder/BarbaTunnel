#include "StdAfx.h"
#include "BarbaCourier.h"
#include "BarbaUtils.h"
#include "BarbaCrypt.h"

BarbaCourier::BarbaCourier(BarbaCourierCreateStrcut* cs)
	: SendEvent(true, true)
	, DisposeEvent(true, false)
{
	this->MaxMessageBuffer = INFINITE;
	this->SentBytesCount = 0;
	this->ReceivedBytesCount = 0;
	this->SessionId = cs->SessionId;
	this->ThreadsStackSize = cs->ThreadsStackSize!=0 ? cs->ThreadsStackSize : 128000; //128KB
	this->MaxConnection = cs->MaxConnenction;
	this->FakeFileMaxSize = cs->FakeFileMaxSize!=0 ? cs->FakeFileMaxSize : 15000000; //15MB
	this->RequestDataKeyName = cs->RequestDataKeyName;
	this->FakeHttpGetTemplate = PrepareFakeRequests(cs->FakeHttpGetTemplate);
	this->FakeHttpPostTemplate = PrepareFakeRequests(cs->FakeHttpPostTemplate);
}

unsigned BarbaCourier::DeleteThread(void* object) 
{
	BarbaCourier* _this = (BarbaCourier*)object;
	_this->Dispose();
	delete _this; 
	return 0;
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
}

void BarbaCourier::Log(LPCTSTR format, ...)
{
	va_list argp;
	va_start(argp, format);
	TCHAR msg[1000];
	_vstprintf_s(msg, format, argp);
	va_end(argp);

	TCHAR msg2[1000];
	_stprintf_s(msg2, _T("BarbaCourier: TID: %4x, SessionId: %x, %s"), GetCurrentThreadId(), this->SessionId, msg);
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
		throw new BarbaException( _T("Message is too big to send!") );

	if (!this->IsDisposing())
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

std::tstring BarbaCourier::PrepareFakeRequests(LPCTSTR request)
{
	std::tstring ret = request;
	StringUtils::ReplaceAll(ret, _T("\r"), _T(""));
	while (StringUtils::ReplaceAll(ret, _T("\n\n"), _T("\n"))!=0); //prevent more than one \n\n
	StringUtils::Trim(ret, '\n');
	ret.append(_T("\n\n"));
	StringUtils::ReplaceAll(ret, _T("\n"), _T("\r\n"));
	return ret;

}

void BarbaCourier::Receive(BYTE* /*buffer*/, size_t /*bufferCount*/)
{
}

void BarbaCourier::ProcessOutgoing(BarbaSocket* barbaSocket, size_t maxBytes)
{
	Log(_T("HTTP connection is ready to send the actual data."));

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
				size_t sendPacketSize = 0;
				BYTE sendPacket[BarbaCourier_MaxMessageLength+2];
				memcpy_s(sendPacket, sizeof (sendPacket)-sendPacketSize, &message->Count, 2);
				sendPacketSize += 2;
				memcpy_s(sendPacket+sendPacketSize, sizeof (sendPacket)-sendPacketSize, message->Buffer, message->Count);
				sendPacketSize += message->Count;

				/*
				//add dummy byes
				size_t minSize = 1400;
				BYTE fakeBuffer[1400];
				size_t fakeSize =  max(minSize-sendPacketSize, 0);
				fakeSize = 0;
				memcpy_s(sendPacket+sendPacketSize, 2, &fakeSize, 2);
				sendPacketSize += 2;
				memcpy_s(sendPacket+sendPacketSize, BarbaCourier_MaxMessageLength-sendPacketSize, fakeBuffer, fakeSize);
				sendPacketSize += fakeSize;
				*/

				int sentCount = barbaSocket->Send(sendPacket, sendPacketSize);
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

void BarbaCourier::ProcessIncoming(BarbaSocket* barbaSocket)
{
	Log(_T("HTTP connection is ready to receive the actual data."));

	size_t receivedBytes = 0;
	bool transferFinish = false;

	//remove message from list
	while(!this->IsDisposing() && !transferFinish)
	{
		u_short messageLen = 0;
		if (barbaSocket->Receive((BYTE*)&messageLen, 2, true)!=2)
			break;

		//check received message len
		if (messageLen>BarbaCourier_MaxMessageLength)
			throw new BarbaException( _T("Out of sync while reading message length!") );

		//read message
		BYTE messageBuf[BarbaCourier_MaxMessageLength];
		//printf("receiving %d\n", messageLen);
		int receiveCount = barbaSocket->Receive(messageBuf, messageLen, true);
		if (receiveCount!=messageLen)
			throw new BarbaException( _T("Out of sync while reading message!") );

		//notify new message
		receivedBytes += receiveCount + 2; //current socket byes
		this->ReceivedBytesCount += receiveCount + 2; //courier byes
		this->Receive(messageBuf, receiveCount);

		/*
		//wait for dummy length
		messageLen = 0;
		if (barbaSocket->Receive((BYTE*)&messageLen, 2, true)!=2)
			break;
		//check dummy length
		if (messageLen>BarbaCourier_MaxMessageLength)
			new BarbaException( _T("Out of sync while reading dummy message length!") );
		//read message
		receiveCount = barbaSocket->Receive(messageBuf, messageLen, true);
		if (receiveCount!=messageLen)
			throw new BarbaException( _T("Out of sync while reading dummy message!") );
		receivedBytes += receiveCount + 2; //current socket byes
		this->ReceivedBytesCount += receiveCount + 2; //courier byes
		*/
	}
}

void BarbaCourier::Sockets_Add(BarbaSocket* socket, bool isOutgoing)
{
	SimpleSafeList<BarbaSocket*>* list = isOutgoing ? &this->OutgoingSockets : &this->IncomingSockets;
	SimpleLock lock(list->GetCriticalSection());
	if (list->GetCount()>=this->MaxConnection)
		throw new BarbaException(_T("Reject new HTTP connection due the maximum connections. MaxUserConnection is: %d."), this->MaxConnection);
	list->AddTail(socket);
	socket->SetNoDelay(true);
}

void BarbaCourier::Sockets_Remove(BarbaSocket* socket, bool isOutgoing)
{
	SimpleSafeList<BarbaSocket*>* list = isOutgoing ? &this->OutgoingSockets : &this->IncomingSockets;
	list->Remove(socket);
	delete socket;
}

void BarbaCourier::SendFakeFileHeader(BarbaSocket* socket, std::vector<BYTE>* fakeFileHeader)
{
	if (fakeFileHeader->empty())
	{
		Log(_T("Could not find fake file data. Fake file header ignored!"));
		return;
	}

	//sending fake file header
	Log(_T("Sending fake file header. HeaderSize: %u."), fakeFileHeader->size());
	if (socket->Send(fakeFileHeader->data(), fakeFileHeader->size())!=(int)fakeFileHeader->size())
		throw new BarbaException(_T("Fake file header does not send!"));
}

void BarbaCourier::WaitForIncomingFakeHeader(BarbaSocket* socket, LPCTSTR httpRequest)
{
	std::tstring requestData = GetRequestDataFromHttpRequest(httpRequest);
	std::tstring fileHeaderSizeStr = BarbaUtils::GetKeyValueFromString(requestData.data(), _T("hlen"));
	u_int fileSize = fileHeaderSizeStr.empty() ? 0 : _tcstoul(fileHeaderSizeStr.data(), 0, 0);

	if (fileSize==0)
	{
		Log(_T("Request does not have fake file header."));
	} 
	else if (fileSize>BarbaCourier_MaxFileHeaderSize)
	{
		throw new BarbaException(_T("Fake file header could not be more than %u size! Requested Size: %u."), BarbaCourier_MaxFileHeaderSize, fileSize);
	}
	else
	{
		std::tstring url = BarbaUtils::GetFileUrlFromHttpRequest(httpRequest);
		Log(_T("Waiting for incoming fake file header. URL: %s, HeaderSize: %u"), url.data(), fileSize);
		
		std::vector<BYTE> buffer(fileSize);
		if (socket->Receive(&buffer.front(), buffer.size(), true)!=(int)buffer.size())
			throw new BarbaException(_T("Could not receive fake file header."));
	}
}

void BarbaCourier::GetFakeFile(TCHAR* filename, u_int* fileSize, std::vector<BYTE>* /*fakeFileHeader*/, bool createNew)
{
	*fileSize = BarbaUtils::GetRandom(this->FakeFileMaxSize/2, this->FakeFileMaxSize); 
	if (createNew)
	{
		u_int fileNameId = BarbaUtils::GetRandom(1, UINT_MAX);
		_ltot_s(fileNameId, filename, MAX_PATH, 32);
	}

}

void BarbaCourier::Crypt(BYTE* /*data*/, size_t /*size*/, bool /*encrypt*/)
{
}


void BarbaCourier::InitFakeRequestVars(std::tstring& src, LPCTSTR host, LPCTSTR fileName, u_int fileSize, u_int fileHeaderSize)
{
	if (host==NULL) host = _T("");
	if (fileName==NULL) fileName = _T("");

	//host
	StringUtils::ReplaceAll(src, _T("{host}"), host);
	
	//filename
	if (fileName!=NULL)
	{
		StringUtils::ReplaceAll(src, _T("{filename}"), fileName);
		StringUtils::ReplaceAll(src, _T("{filetitle}"), BarbaUtils::GetFileTitleFromUrl(fileName));
		StringUtils::ReplaceAll(src, _T("{fileextension}"), BarbaUtils::GetFileExtensionFromUrl(fileName));
	}

	//filesize
	TCHAR fileSizeStr[20];
	_ltot_s(fileSize, fileSizeStr, 10);
	StringUtils::ReplaceAll(src, _T("{filesize}"), fileSizeStr);

	//time
	std::tstring curTime = BarbaUtils::FormatTimeForHttp();
	StringUtils::ReplaceAll(src, _T("{time}"), curTime);
	
	//prepare RequestData: 
	TCHAR requestDataStr[2000]; //hlen=AAFF;session=AABB
	_stprintf_s(requestDataStr, _T("hlen=%u;session=%u;"), fileHeaderSize, this->SessionId);
	//encrypt-data
	std::vector<BYTE> rdataBuffer;
	rdataBuffer.resize(_tcslen(requestDataStr)*sizeof TCHAR);
	memcpy_s(&rdataBuffer.front(), rdataBuffer.size(), requestDataStr, rdataBuffer.size());
	Crypt(&rdataBuffer.front(), rdataBuffer.size(), true);
	//convert to base64
	std::tstring requestData = Base64::encode(&rdataBuffer);

	//RequestData
	std::tstring data;
	data.append(this->RequestDataKeyName);
	data.append(_T("="));
	data.append(requestData);
	StringUtils::ReplaceAll(src, _T("{data}"), data);
}

std::tstring BarbaCourier::GetRequestDataFromHttpRequest(LPCTSTR httpRequest)
{
	try
	{
		std::tstring requestDataEnc = BarbaUtils::GetKeyValueFromString(httpRequest, this->RequestDataKeyName.data());
		if (requestDataEnc.empty())
			return _T("");
		std::vector<BYTE> requestDataBuf;
		Base64::decode(requestDataEnc, requestDataBuf);
		this->Crypt(&requestDataBuf.front(), requestDataBuf.size(), false);
		requestDataBuf.push_back(0);
		requestDataBuf.push_back(0);
		std::string ret = (char*)requestDataBuf.data(); //should be ANSI
		return ret;
	}
	catch (...)
	{
	}

	return _T("");
}

