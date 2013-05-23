#include "StdAfx.h"
#include "BarbaCourierHttpServer.h"
#include "BarbaUtils.h"

BarbaCourierHttpServer::BarbaCourierHttpServer(CreateStrcutHttp* cs)
	: BarbaCourierTcpServer(cs)
{
}


BarbaCourierHttpServer::~BarbaCourierHttpServer(void)
{
}

void BarbaCourierHttpServer::Init(LPCTSTR requestData)
{
	BarbaCourierTcpServer::Init(requestData);
	GetCreateStruct()->RequestMode.Parse(BarbaUtils::GetKeyValueFromString(requestData, _T("HttpRequestMode")));
}

void BarbaCourierHttpServer::WaitForIncomingFileHeader(BarbaSocket* socket, size_t fileHeaderSize)
{
	if (fileHeaderSize==0)
	{
		Log2(_T("Request does not have file header."));
	} 
	else if (fileHeaderSize>BarbaCourier_MaxFileHeaderSize)
	{
		throw new BarbaException(_T("File header could not be more than %u size! Requested Size: %u."), BarbaCourier_MaxFileHeaderSize, fileHeaderSize);
	}
	else
	{
		Log2(_T("Waiting for incoming file header. HeaderSize: %u KB."), fileHeaderSize/1000);

		BarbaBuffer buffer(fileHeaderSize);
		if (socket->Receive(buffer.data(), buffer.size(), true)!=(int)buffer.size())
			throw new BarbaException(_T("Could not receive file header."));
	}
}

void BarbaCourierHttpServer::ServerWorker(ServerWorkerData* serverWorkerData)
{
	BarbaSocket* socket = serverWorkerData->Socket;
	bool isOutgoing = BarbaUtils::GetKeyValueFromString(serverWorkerData->RequestData.data(), _T("Outgoing"), 0)==0;
	size_t transferSize = BarbaUtils::GetKeyValueFromString(serverWorkerData->RequestData.data(), _T("TransferSize"), 0);
	
	try
	{
		if (isOutgoing)
		{
			if (GetRequestMode()->BombardGet)
			{
				//report new connection
				Log2(_T("HTTP Bombard GET added. Port: %d, Connections Count: %d."), socket->GetLocalPort(), OutgoingSockets.GetCount());

				//process socket until filesize transfered
				ProcessOutgoing(socket, transferSize);

				//finish current connection
				Log2(_T("HTTP Bombard GET Transfer Completed."));
			}
			else
			{
				//report new connection
				Log2(_T("HTTP GET added. Port: %d, Connections Count: %d."), socket->GetLocalPort(), OutgoingSockets.GetCount());

				//send file header
				BarbaBuffer fileHeader;
				std::tstring requestUrl = BarbaUtils::GetFileUrlFromHttpRequest(serverWorkerData->RequestString.data());
				size_t remainBytes = SendGetReply(socket, requestUrl.data(), transferSize, &fileHeader);

				//sending file header
				if (fileHeader.size()!=0)
				{
					Log2(_T("Sending File Header! HeaderSize: %d KB"), fileHeader.size()/1000);
					socket->Send((BYTE*)fileHeader.data(), fileHeader.size());
				}

				remainBytes -= fileHeader.size();

				//process socket until socket closed or or file transfered
				ProcessOutgoing(socket, remainBytes);

				//report
				Log2(_T("HTTP GET completed. File sent."));
			}
		}
		else
		{
			if (GetRequestMode()->BombardPost || GetRequestMode()->BombardPostReply)
			{
				//report new connection
				Log2(_T("HTTP Bombard %s added. Port: %d, Connections Count: %d."), GetRequestMode()->BombardPostReply ? _T("POST & REPLY") : _T("POST") , socket->GetLocalPort(), IncomingSockets.GetCount());

				//reply to initial request. In initial send always even if IsBombardPostReply is false
				SendPostReply(socket);

				//process socket until socket closed
				ProcessIncoming(socket, transferSize);

				//finish current connection
				Log2(_T("HTTP Bombard POST Transfer Completed."));
			}
			else
			{
				//report new connection
				Log2(_T("HTTP POST added. Port: %d, Connections Count: %d."), socket->GetLocalPort(), IncomingSockets.GetCount());

				//Get Initial data
				size_t fileHeaderSize = BarbaUtils::GetKeyValueFromString(serverWorkerData->RequestData.data(), _T("FileHeaderSize"), 0);

				//wait for incoming file header
				Log2(_T("Receiving file %u KB."), transferSize/1000);
				WaitForIncomingFileHeader(socket, fileHeaderSize);

				//process socket until socket closed or file transfered
				ProcessIncoming(socket, transferSize-fileHeaderSize);

				//Send POST Reply that file downloaded
				SendPostReply(socket);

				//File received.
				Log2(_T("HTTP POST completed. File received."));
			}
		}
	}
	catch(BarbaException* er)
	{
		Log2(_T("Error: %s"), er->ToString());
		delete er;
	}
	catch(...)
	{
		Log2(_T("Unknown Error!"));
	}

	//remove socket from store
	Sockets_Remove(socket, isOutgoing);
	Log2(_T("HTTP %s connection removed. Connections Count: %d."), isOutgoing ? _T("GET") : _T("POST"), isOutgoing ? OutgoingSockets.GetCount() : IncomingSockets.GetCount());
}

void BarbaCourierHttpServer::BeforeSendMessage(BarbaSocket* /*barbaSocket*/, BarbaBuffer* messageBuffer) 
{
	if (GetRequestMode()->BombardGet)
	{
		BarbaBuffer buffer;
		GetGetReplyBombard(messageBuffer->size(), &buffer);
		buffer.append(messageBuffer);
		messageBuffer->assign(&buffer);
		Log3(_T("Sending with GET request. Count: %d bytes."), messageBuffer->size());
	}
	else
	{
		Log3(_T("Sending to GET channel. Count: %d bytes."), messageBuffer->size());
	}
}

void BarbaCourierHttpServer::AfterSendMessage(BarbaSocket* barbaSocket, bool isTransferFinished)
{
	//waiting for another get request in GET bombard mode
	if (GetRequestMode()->BombardGet && !isTransferFinished)
	{
		std::string header = barbaSocket->ReadHttpRequest();
		size_t contentLength = BarbaUtils::GetKeyValueFromString(header.data(), _T("Content-Length"), 0);
		if (contentLength>0)
		{
			ProcessIncomingMessage(barbaSocket, 0, contentLength);
			Log3(_T("Receiving from GET request payload. Payload: %d bytes."), contentLength);
		}
	}
}

void BarbaCourierHttpServer::BeforeReceiveMessage(BarbaSocket* barbaSocket, size_t* chunkSize)
{
	if (GetRequestMode()->BombardPost)
	{
		std::string header = barbaSocket->ReadHttpRequest();
		if (header.empty())
			throw new BarbaException( _T("Server does not accept request!") );
		*chunkSize = BarbaUtils::GetKeyValueFromString(header.data(), _T("Content-Length"), 0);
		Log3(_T("Receiving from POST request. Count: %d bytes."), *chunkSize);
	}
}

void BarbaCourierHttpServer::AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength, bool isTransferFinished)
{
	if (GetRequestMode()->BombardPostReply && !isTransferFinished)
	{
		BarbaArray<Message*> messages;
		try
		{
			//attach some message to get request
			BarbaBuffer buffer;
			if (GetRequestMode()->BombardPostReplyPayload)
			{
				GetMessages(messages);
				ProcessOutgoingMessages(messages, 0, 0, &buffer);
				if (buffer.size()>0) 
					Log3("Sending with POST-REPLY payload. Payload: %d bytes.", buffer.size());
			}
			else
			{
				Log3("Receiving with POST request. Payload: %d bytes.", messageLength);
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
		Log3("Receiving from POST channel. Count: %d bytes.", messageLength);
	}
}

void BarbaCourierHttpServer::SendPostReply(BarbaSocket* socket)
{
	BarbaBuffer content;
	SendPostReply(socket, &content);
}

void BarbaCourierHttpServer::SendPostReply(BarbaSocket* socket, BarbaBuffer* content)
{
	if ( IsDisposing() ) 
		throw new BarbaException(_T("Could not send request while disposing!"));

	//process replyPayload
	std::tstring postReply = GetHttpPostReplyRequest(GetRequestMode()->BombardPost);
	BarbaUtils::UpdateHttpRequest(&postReply, GetCreateStruct()->HostName.data(), NULL, _T("application/octet-stream"), content->size(), NULL);

	//send buffer
	BarbaBuffer buffer;
	buffer.reserve(postReply.length() + content->size());
	buffer.append((char*)postReply.data(), postReply.size());
	buffer.append(content);

	//log
	if (GetRequestMode()->BombardPostReply) Log3(_T("Sending Post Reply! Payload: %d bytes."), content->size());
	else Log2(_T("Sending Post Reply!"));

	//send
	socket->Send(buffer.data(), buffer.size());
}

size_t BarbaCourierHttpServer::SendGetReply(BarbaSocket* socket, LPCTSTR fileUrl, size_t fileSize, BarbaBuffer* fileHeader)
{
	if ( IsDisposing() ) throw new BarbaException(_T("Could not send request while disposing!"));
	std::tstring requestFile = BarbaUtils::GetFileNameFromUrl(fileUrl);

	TCHAR filename[MAX_PATH];
	_tcscpy_s(filename, requestFile .data());
	std::tstring contentType;
	GetFakeFile(filename, &contentType, fileHeader);
	size_t fakeFileHeaderSize = fileHeader!=NULL ? fileHeader->size() : 0;

	std::tstring reply = GetHttpGetReplyRequest(false);
	std::tstring requestData;
	BarbaUtils::SetKeyValue(&requestData, _T("FileHeaderSize"), (int)fileHeader->size());
	BarbaUtils::UpdateHttpRequest(&reply, GetCreateStruct()->HostName.data(), filename, contentType.data(), fileSize, RequestData_ToString(requestData).data());

	Log2(_T("Sending GET reply! File: %s (%u KB)."), filename, fileSize/1000, fakeFileHeaderSize);
	std::string replyA = replyA;
	socket->Send((BYTE*)reply.data(), reply.size());

	return fileSize;
}

void BarbaCourierHttpServer::GetGetReplyBombard(size_t dataLength, BarbaBuffer* requestBuffer)
{
	std::tstring reply = GetHttpGetReplyRequest(true);
	BarbaUtils::UpdateHttpRequest(&reply, GetCreateStruct()->HostName.data(), NULL, _T("application/octet-stream"), dataLength, NULL);
	requestBuffer->append((char*)reply.data(), reply.size());
}

