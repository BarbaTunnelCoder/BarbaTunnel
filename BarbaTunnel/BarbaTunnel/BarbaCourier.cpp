#include "StdAfx.h"
#include "BarbaCourier.h"
#include "BarbaUtils.h"
#include "BarbaCrypt.h"

BarbaCourier::BarbaCourier(BarbaCourier::CreateStrcutBag* cs)
	: SendEvent(true, true)
	, DisposeEvent(true, false)
{
	this->MaxMessageBuffer = INFINITE;
	this->SentBytesCount = 0;
	this->ReceivedBytesCount = 0;
	this->CreateStruct = *cs;
	this->LastReceivedTime = 0;
	this->LastSentTime = 0;
	RefreshParameters();
}

void BarbaCourier::RefreshParameters()
{
	if (this->CreateStruct.KeepAliveInterval!=0 && this->CreateStruct.KeepAliveInterval<BARBA_HttpKeepAliveIntervalMin) this->CreateStruct.KeepAliveInterval = BARBA_HttpKeepAliveIntervalMin;
	if (this->CreateStruct.ThreadsStackSize==0) this->CreateStruct.ThreadsStackSize = BARBA_SocketThreadStackSize; 
	if (this->CreateStruct.FakeFileMaxSize==0) this->CreateStruct.FakeFileMaxSize = BARBA_HttpFakeFileMaxSize; 
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

void BarbaCourier::LogImpl(int level, LPCTSTR format, va_list _ArgList)
{
	TCHAR* msg = new TCHAR[1000];
	_vstprintf_s(msg, 1000, format, _ArgList);
	
	TCHAR* msg2 = new TCHAR[1000];
	_stprintf_s(msg2, 1000, _T("BarbaCourier: TID: %4x, SessionId: %x, %s"), GetCurrentThreadId(), this->CreateStruct.SessionId, msg);

	va_list emptyList = {0};
	BarbaLogImpl(level, msg2, emptyList);
	delete msg;
	delete msg2;
}

void BarbaCourier::Log2(LPCTSTR format, ...) { va_list argp; va_start(argp, format); LogImpl(2, format, argp); va_end(argp); }
void BarbaCourier::Log3(LPCTSTR format, ...) { va_list argp; va_start(argp, format); LogImpl(3, format, argp); va_end(argp); }

BarbaCourier::~BarbaCourier(void)
{
}

HANDLE BarbaCourier::Delete()
{
	Log2(_T("BarbaCourier disposing."));
	DisposeEvent.Set();
	return (HANDLE)_beginthreadex(0, this->CreateStruct.ThreadsStackSize, DeleteThread, (void*)this, 0, NULL);
}

void BarbaCourier::Dispose()
{
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
	Log2(_T("BarbaCourier disposed."));
}

void BarbaCourier::Send(BarbaBuffer* data)
{
	Send(new Message(data));
}

void BarbaCourier::Send(Message* message, bool highPriority)
{
	if (message->GetDataSize()>BarbaCourier_MaxMessageLength)
	{
		delete message;
		throw new BarbaException( _T("Message is too big to send! It should not be greater than %u bytes."), BarbaCourier_MaxMessageLength);
	}

	BarbaArray<Message*> messages(&message, 1);
	Send(messages, highPriority);
}

void BarbaCourier::Send(BarbaArray<Message*>& messages, bool highPriority)
{
	//check is any message
	if (messages.size()==0)
		return;

	//check is disposing
	if (this->IsDisposing())
	{
		for (int i=0; i<(int)messages.size(); i++)
			delete messages[i];
		messages.clear();
		return;
	}

	//wait till pulse set
	SimpleLock lock(&SendEventCS);

	for (int i=0; i<(int)messages.size(); i++)
	{
		if (messages[i]->GetDataSize()>BarbaCourier_MaxMessageLength)
			Log2(_T("Message is too big to send! It should not be greater than %u bytes."), BarbaCourier_MaxMessageLength);
		else if (highPriority)
			Messages.AddHead(messages[i]);
		else
			Messages.AddTail(messages[i]);
	}

	//check maximum send buffer and remove old messages from head. New messages exists in tail
	while (Messages.GetCount()>MaxMessageBuffer)
	{
		Message* message = Messages.RemoveHead();
		if (message!=NULL)
			delete message;
	}

	SendEvent.Set();
}

void BarbaCourier::Receive(BarbaBuffer* /*data*/)
{
}

void BarbaCourier::GetMessages(BarbaArray<Message*>& messages)
{
	messages.reserve(this->Messages.GetCount());
	Message* message = this->Messages.RemoveHead();
	while (message!=NULL)
	{
		messages.append(message);
		message = this->Messages.RemoveHead();
	}
}

BarbaCourier::Message* BarbaCourier::GetMessage()
{
	return Messages.RemoveHead();
}

void BarbaCourier::ProcessOutgoing(BarbaSocket* barbaSocket, size_t maxBytes)
{
	Log2(_T("HTTP connection is ready to send the actual data. TransferSize: %d KB."), maxBytes/1000);

	size_t sentBytes = 0;
	bool transferFinish = false;

	//remove message from list
	while(!this->IsDisposing() && !transferFinish)
	{
		DWORD waitResult = SendEvent.Wait(2*1000);

		//check is connection still writable
		if (waitResult==WAIT_TIMEOUT && !barbaSocket->IsWritable())
			break;

		//post all message
		BarbaArray<Message*> messages;
		GetMessages(messages);

		//send KeepAlive from server to client
		//if lastKeepAliveTime > this->LastReceivedTime it mean keepAlive already sent and it does not need more until next receive
		if (IsServer() && messages.size()==0 &&
			this->CreateStruct.KeepAliveInterval!=0 && 
			barbaSocket->GetLastSentTime() < this->LastReceivedTime &&
			GetTickCount() > (barbaSocket->GetLastSentTime() + this->CreateStruct.KeepAliveInterval) )
		{
			messages.append(new Message());
		}

		while (messages.size()>0)
		{
			try
			{
				//prepare packet
				BarbaBuffer packet;
				ProcessOutgoingMessages(messages, sentBytes, maxBytes-sentBytes, &packet);
				
				//save transfer count before BeforeSendMessage change packet
				size_t transferCount = packet.size();

				//Notify message going to send
				BeforeSendMessage(barbaSocket, &packet);

				//sending buffer
				barbaSocket->Send(packet.data(), packet.size());
				
				//calculate stats
				this->SentBytesCount += transferCount; //courier bytes
				sentBytes += transferCount; //current socket bytes
				this->LastSentTime = GetTickCount();

				//check is transfer finished
				transferFinish = sentBytes>=maxBytes;

				//delete sent message and get next message
				for (int i=0; i<(int)messages.size(); i++)
					delete messages[i];
				messages.clear();

				//notify message sent
				AfterSendMessage(barbaSocket, transferFinish);

				//get next messages
				if (!transferFinish)
					GetMessages(messages);
			}
			catch(...)
			{
				//bring back message to list if failed
				Send(messages, true);
				throw;
			}
		}

		//reset if there is no message
		SimpleLock lock(&SendEventCS);
		if (this->Messages.IsEmpty() && !this->IsDisposing())
			SendEvent.Reset();
		lock.Unlock();
	}
}

void BarbaCourier::ProcessIncoming(BarbaSocket* barbaSocket, size_t maxBytes)
{
	Log2(_T("HTTP connection is ready to receive the actual data. TransferSize: %d KB."), maxBytes/1000);

	size_t receivedBytes = 0;
	bool isTransferFinished = receivedBytes>=maxBytes;

	//remove message from list
	while(!this->IsDisposing() && !isTransferFinished)
	{
		//notify receiving message
		size_t chunkSize;
		BeforeReceiveMessage(barbaSocket, &chunkSize);

		size_t readedBytes = ProcessIncomingMessage(barbaSocket, receivedBytes, chunkSize);
		receivedBytes += readedBytes;
		this->ReceivedBytesCount += readedBytes;
		this->LastReceivedTime = GetTickCount();

		//check is transfer finished
		isTransferFinished = receivedBytes>=maxBytes;

		//notify message received
		AfterReceiveMessage(barbaSocket, readedBytes, isTransferFinished);
	}
}

void BarbaCourier::ProcessOutgoingMessages(BarbaArray<Message*>& messages, size_t cryptIndex, size_t maxPacketSize, BarbaBuffer* packet)
{
	// don't do anything if there is no message
	if (messages.size()==0)
		return;

	//size_t orgMaxPacketSize = maxPacketSize;
	if (maxPacketSize == 0) 
		maxPacketSize = (size_t)-1;

	//find packet that should send
	size_t packetSize = 0;
	BarbaArray<Message*> includeMessages;
	BarbaArray<Message*> excludeMessages;
	includeMessages.reserve(messages.size());
	excludeMessages.reserve(messages.size());
	for (int i=0; i<(int)messages.size(); i++)
	{
		size_t messageSize = messages[i]->GetDataSize() + 3;
		if ( (packetSize + messageSize) <= maxPacketSize ) 
		{
			packetSize += messageSize;
			includeMessages.append(messages[i]);
		}
		else
		{
			excludeMessages.append(messages[i]);
		}
	}

	//bring back excludeMessages
	Send(excludeMessages, true);

	//add fake message to reach minumum size
	size_t minPacketSize = min(BARBA_HttpFakePacketMaxSize, this->CreateStruct.FakePacketMinSize);
	//if no packet fit in maxPacketSize then fill rest of buffer with 0 byte
	if (includeMessages.size()==0 && messages.size()!=0 && maxPacketSize<2000) minPacketSize = maxPacketSize; 
	//allocate fakeBuffer with zero value
	size_t fakeSize = max(0, (int)(minPacketSize - packetSize));
	BarbaBuffer fakeBuffer(fakeSize);
	packetSize += fakeBuffer.size();

	//prepare sending message
	//Packet Format: MessageType|MessageSize|Message
	packet->reserve(packetSize);
	for (int i=0; i<(int)includeMessages.size(); i++)
	{
		Message* message = includeMessages[i];
		size_t messageSize = message->GetDataSize();
		packet->append(1);
		packet->append(&messageSize, 2);
		packet->append(message->GetData(), message->GetDataSize());
	}

	//fill rest of packet with zero
	packet->append(&fakeBuffer);

	//encrypt packet
	Crypt(packet, cryptIndex, true);

	//copy includeMessages to messages
	messages.assign(&includeMessages);
}

size_t BarbaCourier::ProcessIncomingMessage(BarbaSocket* barbaSocket, size_t cryptIndex, size_t chunkSize)
{
	size_t receivedBytes = 0;
	if (chunkSize==0) chunkSize = 1; //run loop code just one time, i don't like do-while

	while (receivedBytes<chunkSize)
	{
		//read message type; just ignore if zero
		BYTE messageType = 0;
		if (barbaSocket->Receive((BYTE*)&messageType, 1, true)!=1)
			throw new BarbaException( _T("Out of sync while reading message length!") );
		Crypt((BYTE*)&messageType, 1, cryptIndex + receivedBytes, false);
		receivedBytes +=1;
		if (messageType==0)
			continue;

		//read message size
		u_short messageLen = 0;
		if (barbaSocket->Receive((BYTE*)&messageLen, 2, true)!=2)
			throw new BarbaException( _T("Out of sync while reading message length!") );
		Crypt((BYTE*)&messageLen, 2, cryptIndex + receivedBytes, false);
		receivedBytes += 2; 	

		//read message data
		if (messageLen!=0)
		{
			BarbaBuffer messageBuf(messageLen);
			size_t receiveCount = barbaSocket->Receive(messageBuf.data(), messageBuf.size(), true);
			if (receiveCount!=messageBuf.size())
				throw new BarbaException( _T("Out of sync while reading message!") );
			Crypt(&messageBuf, cryptIndex + receivedBytes, false); 
			receivedBytes += receiveCount;

			//notify new message
			this->Receive(&messageBuf);
		}
	}

	return receivedBytes;
}


void BarbaCourier::BeforeSendMessage(BarbaSocket* /*barbaSocket*/, BarbaBuffer* /*messageBuffer*/) {}
void BarbaCourier::AfterSendMessage(BarbaSocket* /*barbaSocket*/, bool /*isTransferFinished*/)  {}
void BarbaCourier::BeforeReceiveMessage(BarbaSocket* /*barbaSocket*/, size_t* /*chunkSize*/)  {}
void BarbaCourier::AfterReceiveMessage(BarbaSocket* /*barbaSocket*/, size_t /*messageLength*/, bool /*isTransferFinished*/)  {}

void BarbaCourier::Sockets_Add(BarbaSocket* socket, bool isOutgoing)
{
	if (this->CreateStruct.MaxConnection==0)
		throw new BarbaException(_T("Reject new HTTP connection due the maximum connections. MaxUserConnection is: %d."), this->CreateStruct.MaxConnection);

	SimpleSafeList<BarbaSocket*>* list = isOutgoing ? &this->OutgoingSockets : &this->IncomingSockets;
	if (list->GetCount()>=this->CreateStruct.MaxConnection)
	{
		BarbaSocket* socket = list->RemoveHead();
		if (socket!=NULL) 
			socket->Close(); //close head collection
	}

	list->AddTail(socket);
	socket->SetNoDelay(true);

	//Receive Timeout for in-comming 
	if (this->CreateStruct.ConnectionTimeout!=0)
		socket->SetReceiveTimeOut(this->CreateStruct.ConnectionTimeout);

	//KeepAliveTimeout
	if (this->CreateStruct.KeepAliveInterval!=0)
		socket->SetSendTimeOut(this->CreateStruct.KeepAliveInterval); //I don't know why it doesn't any effect, anyway I set it
}

void BarbaCourier::Sockets_Remove(BarbaSocket* socket, bool isOutgoing)
{
	SimpleSafeList<BarbaSocket*>* list = isOutgoing ? &this->OutgoingSockets : &this->IncomingSockets;
	list->Remove(socket);
	delete socket;
}

void BarbaCourier::Crypt(BarbaBuffer* data, size_t index, bool encrypt)
{
	Crypt(data->data(), data->size(), index, encrypt);
}

void BarbaCourier::Crypt(BYTE* /*data*/, size_t /*dataSize*/, size_t /*index*/, bool /*encrypt*/)
{
}

std::tstring BarbaCourier::CreateRequestParam(size_t transferSize, bool outgoing, LPCTSTR other)
{
	//prepare RequestData: 
	CHAR* requestDataStr = new CHAR[2000]; //filesize=1234;fileheadersize=1234;session=1234;keepalive=1234;bombardmode=postreply;
	sprintf_s(requestDataStr, 2000, "transferSize:%u;session:%u;packetMinSize:%u;keepalive:%u;bombardMode:%s,outgoing=%d;%s;", 
		transferSize, 
		this->CreateStruct.SessionId, 
		this->CreateStruct.FakePacketMinSize, 
		this->CreateStruct.KeepAliveInterval, 
		this->CreateStruct.HttpBombardMode.data(),
		(int)outgoing,
		other);

	//encrypt-data
	BarbaBuffer rdataBuffer((BYTE*)requestDataStr, _tcslen(requestDataStr));
	Crypt(&rdataBuffer, 0, true);
	
	//convert to base64
	std::tstring requestData = Base64::encode(rdataBuffer.data(), rdataBuffer.size());

	//return RequestData
	std::tstring data;
	data.append(this->CreateStruct.RequestDataKeyName);
	data.append(_T(":"));
	data.append(requestData);

	delete requestDataStr;
	return data;
}