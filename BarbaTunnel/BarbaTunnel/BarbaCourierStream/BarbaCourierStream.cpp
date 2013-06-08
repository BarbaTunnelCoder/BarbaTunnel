#include "StdAfx.h"
#include "BarbaCourierStream.h"
#include "BarbaUtils.h"
#include "BarbaCrypt.h"


BarbaCourierStream::BarbaCourierStream(BarbaCourierStream::CreateStrcut* cs)
	: SendEvent(true, true)
	, DisposeEvent(true, false)
{
	MaxMessageBuffer = INFINITE;
	SentBytesCount = 0;
	ReceivedBytesCount = 0;
	_CreateStruct = cs;
	LastReceivedTime = 0;
	LastSentTime = 0;
}

void BarbaCourierStream::Init()
{
	RequestDataKeyName = CreateRequestDataKeyName();
}

BarbaCourierStream::~BarbaCourierStream(void)
{
	delete _CreateStruct;
}

void BarbaCourierStream::StartKeepAliveThread()
{
	if (GetCreateStruct()->KeepAliveInterval!=0)
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, CheckKeepAliveThread, this, 0, NULL));
}

unsigned int __stdcall BarbaCourierStream::CheckKeepAliveThread(void* barbaCourier)
{
	BarbaCourierStream* _this = (BarbaCourierStream*)barbaCourier;
	while (_this->DisposeEvent.Wait(2000)==WAIT_TIMEOUT)
	{
		_this->CheckKeepAlive();
	}
	return 0;
}

void BarbaCourierStream::CheckKeepAlive()
{
	//close connection that does not receive keep alive
	if (GetCreateStruct()->KeepAliveInterval==0 || LastSentTime==0)
		return; //keep live not enabled or no data sent yet

	SimpleSafeList<BarbaSocket*>* list = &IncomingSockets;
	SimpleSafeList<BarbaSocket*>::AutoLockBuffer autoLockBuf(list);
	BarbaSocket** sockets = autoLockBuf.GetBuffer();
	for (size_t i=0; i<list->GetCount(); i++)
	{
		if (sockets[i]->IsOpen() &&
			sockets[i]->GetLastReceivedTime() < LastSentTime &&
			GetTickCount() > (sockets[i]->GetLastReceivedTime() + GetCreateStruct()->KeepAliveInterval*2) )
		{
			Log2(_T("Connection closed due to keep alive timeout!"));
			sockets[i]->Close();
		}
	}
}

unsigned BarbaCourierStream::DeleteThread(void* object) 
{
	BarbaCourierStream* _this = (BarbaCourierStream*)object;
	_this->Dispose();
	delete _this; 
	return 0;
}

void BarbaCourierStream::CloseSocketsList(SimpleSafeList<BarbaSocket*>* list)
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

void BarbaCourierStream::LogImpl(int level, LPCTSTR format, va_list _ArgList)
{
	TCHAR* msg = new TCHAR[1000];
	_vstprintf_s(msg, 1000, format, _ArgList);
	
	TCHAR* msg2 = new TCHAR[1000];
	_stprintf_s(msg2, 1000, _T("BarbaCourierStream: TID: %4x, SessionId: %x, %s"), GetCurrentThreadId(), this->GetCreateStruct()->SessionId, msg);

	va_list emptyList = {0};
	BarbaLogImpl(level, msg2, emptyList);
	delete msg;
	delete msg2;
}

void BarbaCourierStream::Log2(LPCTSTR format, ...) { va_list argp; va_start(argp, format); LogImpl(2, format, argp); va_end(argp); }
void BarbaCourierStream::Log3(LPCTSTR format, ...) { va_list argp; va_start(argp, format); LogImpl(3, format, argp); va_end(argp); }

HANDLE BarbaCourierStream::Delete()
{
	Log2(_T("BarbaCourierStream disposing."));
	DisposeEvent.Set();
	return (HANDLE)_beginthreadex(0, BARBA_SocketThreadStackSize, DeleteThread, (void*)this, 0, NULL);
}

void BarbaCourierStream::Dispose()
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
	Log2(_T("BarbaCourierStream disposed."));
}

void BarbaCourierStream::Send(BarbaBuffer* data)
{
	Send(new Message(data));
}

void BarbaCourierStream::Send(Message* message, bool highPriority)
{
	if (message->GetDataSize()>BarbaCourier_MaxMessageLength)
	{
		delete message;
		throw new BarbaException( _T("Message is too big to send! It should not be greater than %u bytes."), BarbaCourier_MaxMessageLength);
	}

	BarbaArray<Message*> messages(&message, 1);
	Send(messages, highPriority);
}

void BarbaCourierStream::Send(BarbaArray<Message*>& messages, bool highPriority)
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

void BarbaCourierStream::Receive(BarbaBuffer* /*data*/)
{
}

void BarbaCourierStream::GetMessages(BarbaArray<Message*>& messages)
{
	messages.reserve(this->Messages.GetCount());
	Message* message = this->Messages.RemoveHead();
	while (message!=NULL)
	{
		messages.append(message);
		message = this->Messages.RemoveHead();
	}
}

BarbaCourierStream::Message* BarbaCourierStream::GetMessage()
{
	return Messages.RemoveHead();
}

void BarbaCourierStream::ProcessOutgoing(BarbaSocket* barbaSocket, size_t maxBytes)
{
	Log2(_T("Connection is ready to send the actual data. TransferSize: %d KB."), maxBytes/1000);

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
			this->GetCreateStruct()->KeepAliveInterval!=0 && 
			barbaSocket->GetLastSentTime() < this->LastReceivedTime &&
			GetTickCount() > (barbaSocket->GetLastSentTime() + this->GetCreateStruct()->KeepAliveInterval) )
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

void BarbaCourierStream::ProcessIncoming(BarbaSocket* barbaSocket, size_t maxBytes)
{
	Log2(_T("Connection is ready to receive the actual data. TransferSize: %d KB."), maxBytes/1000);

	size_t receivedBytes = 0;
	bool isTransferFinished = receivedBytes>=maxBytes;

	//remove message from list
	while(!this->IsDisposing() && !isTransferFinished)
	{
		//notify receiving message
		size_t chunkSize = 0;;
		BeforeReceiveMessage(barbaSocket, &chunkSize);

		//do not process All Chunk if inherited class specify chunkSize
		bool processAllChunk = chunkSize!=0;
		if (!processAllChunk) chunkSize = maxBytes-receivedBytes;

		size_t readedBytes = ProcessIncomingMessage(barbaSocket, receivedBytes, chunkSize, processAllChunk);
		receivedBytes += readedBytes;
		this->ReceivedBytesCount += readedBytes;
		this->LastReceivedTime = GetTickCount();

		//check is transfer finished
		isTransferFinished = receivedBytes>=maxBytes;

		//notify message received
		AfterReceiveMessage(barbaSocket, readedBytes, isTransferFinished);
	}
}

void BarbaCourierStream::ProcessOutgoingMessages(BarbaArray<Message*>& messages, size_t cryptIndex, size_t maxPacketSize, BarbaBuffer* packet)
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
	size_t minPacketSize = min(BARBA_MinPacketSizeLimit, GetCreateStruct()->MinPacketSize);
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

size_t BarbaCourierStream::ProcessIncomingMessage(BarbaSocket* barbaSocket, size_t cryptIndex, size_t chunkSize, bool processAllChunk)
{
	size_t receivedBytes = 0;
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

		if (!processAllChunk)
			break;
	}

	return receivedBytes;
}


void BarbaCourierStream::BeforeSendMessage(BarbaSocket* /*barbaSocket*/, BarbaBuffer* messageBuffer)
{
	Log3(_T("Sending %d bytes"), messageBuffer->size());
}

void BarbaCourierStream::AfterSendMessage(BarbaSocket* /*barbaSocket*/, bool /*isTransferFinished*/)
{
}

void BarbaCourierStream::BeforeReceiveMessage(BarbaSocket* /*barbaSocket*/, size_t* /*chunkSize*/)
{
}

void BarbaCourierStream::AfterReceiveMessage(BarbaSocket* /*barbaSocket*/, size_t messageLength, bool /*isTransferFinished*/) 
{
	Log3(_T("Received %d bytes"), messageLength);
}

void BarbaCourierStream::Sockets_Add(BarbaSocket* socket, bool isOutgoing)
{
	//check maximum connection
	SimpleSafeList<BarbaSocket*>* list = isOutgoing ? &OutgoingSockets : &IncomingSockets;
	if (list->GetCount()>=this->GetCreateStruct()->MaxConnections)
	{
		Log2(_T("Remove old connection due the maximum connections. MaxUserConnection is: %d."), GetCreateStruct()->MaxConnections);
		BarbaSocket* socket = list->RemoveHead();
		if (socket!=NULL) 
			socket->Close(); //close head collection
	}

	list->AddTail(socket);
	socket->SetNoDelay(true);

	//Receive Timeout for in-comming 
	if (this->GetCreateStruct()->ConnectionTimeout!=0)
		socket->SetReceiveTimeOut(this->GetCreateStruct()->ConnectionTimeout);

	//KeepAliveTimeout
	if (this->GetCreateStruct()->KeepAliveInterval!=0)
		socket->SetSendTimeOut(this->GetCreateStruct()->KeepAliveInterval); //I don't know why it doesn't any effect, anyway I set it
}

void BarbaCourierStream::Sockets_Remove(BarbaSocket* socket, bool isOutgoing)
{
	SimpleSafeList<BarbaSocket*>* list = isOutgoing ? &this->OutgoingSockets : &this->IncomingSockets;
	list->Remove(socket);
	delete socket;
}

void BarbaCourierStream::Crypt(BarbaBuffer* data, size_t index, bool encrypt)
{
	Crypt(data->data(), data->size(), index, encrypt);
}

void BarbaCourierStream::Crypt(BYTE* /*data*/, size_t /*dataSize*/, size_t /*index*/, bool /*encrypt*/)
{
}

std::tstring BarbaCourierStream::RequestData_ToString(std::tstring requestData)
{
	//encrypt-data
	BarbaBuffer rdataBuffer((BYTE*)requestData.data(), requestData.size());
	Crypt(&rdataBuffer, 0, true);
	
	//convert to base64
	std::tstring requestDataEnc = Base64::encode(rdataBuffer.data(), rdataBuffer.size());

	//return RequestData
	std::tstring data;
	data.append(RequestDataKeyName);
	data.append(_T(":"));
	data.append(requestDataEnc);

	return data;
}

std::tstring BarbaCourierStream::RequestData_FromString(std::tstring requestString)
{
	try
	{
		std::tstring requestDataEnc = BarbaUtils::GetKeyValueFromString(requestString.data(), RequestDataKeyName.data());
		if (requestDataEnc.empty())
			return _T("");

		std::vector<BYTE> decodeBuffer;
		Base64::decode(requestDataEnc, decodeBuffer);
		BarbaBuffer requestDataBuf(decodeBuffer.data(), decodeBuffer.size());
		this->Crypt(&requestDataBuf, 0, false);
		requestDataBuf.append((BYTE)0);
		requestDataBuf.append((BYTE)0);
		std::string ret = (char*)requestDataBuf.data(); //should be ANSI
		return ret;
	}
	catch (...)
	{
	}

	return _T("");
}

std::tstring BarbaCourierStream::CreateRequestDataKeyName()
{
	std::string keyName = "RequestDataKey";
	BarbaBuffer keyBuffer((BYTE*)keyName.data(), keyName.size());
	Crypt(&keyBuffer, 0, true);
	std::tstring ret = Base64::encode(keyBuffer.data(), keyBuffer.size());
	StringUtils::ReplaceAll(ret, "=", "");
	StringUtils::ReplaceAll(ret, "/", "");
	return ret;
}
