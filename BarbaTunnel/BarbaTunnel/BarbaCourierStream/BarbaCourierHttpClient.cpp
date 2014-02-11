#include "StdAfx.h"
#include "BarbaCourierHttpClient.h"
#include "BarbaUtils.h"


BarbaCourierHttpClient::BarbaCourierHttpClient(CreateStrcutHttp* cs)
	: BarbaCourierTcpClient(cs)
{
}

BarbaCourierHttpClient::~BarbaCourierHttpClient()
{
}

std::tstring BarbaCourierHttpClient::CreateRequestString(bool outgoing, size_t transferSize, size_t fileHeaderSize)
{
	std::tstring req;
	BarbaUtils::SetKeyValue(&req, _T("FileHeaderSize"), (int)fileHeaderSize);
	BarbaUtils::SetKeyValue(&req, _T("HttpRequestMode"), GetRequestMode()->ToString().data());
	return BarbaCourierTcpClient::CreateRequestString(outgoing, transferSize, req.data());
}


void BarbaCourierHttpClient::GetPostRequestBombard(size_t dataLength, BarbaBuffer* requestBuffer)
{
	TCHAR filename[MAX_PATH];
	std::tstring contentType;
	GetFakeFile(filename, &contentType, NULL, NULL, true);

	//use non bombard request for initialize request
	std::tstring request = GetHttpPostTemplate(true);
	BarbaUtils::UpdateHttpRequest(&request, GetCreateStruct()->HostName.data(), filename, contentType.data(), dataLength, NULL);

	//send request
	requestBuffer->append((char*)request.data(), request.size());
}

size_t BarbaCourierHttpClient::SendPostRequest(BarbaSocket* socket, bool initBombard)
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
	std::tstring httpRequest = GetHttpPostTemplate(false);
	std::tstring requestData = CreateRequestString(true, fileSize, fileHeader.size());
	BarbaUtils::UpdateHttpRequest(&httpRequest, GetCreateStruct()->HostName.data(), filename, contentType.data(), initBombard ? 0 : fileSize, requestData.data());

	//send request
	socket->Send((BYTE*)httpRequest.data(), httpRequest.size());

	//sending file header in no bombard mode
	if (fileHeader.size()!=0)
	{
		Log2(_T("Sending File Header! HeaderSize: %d KB"), fileHeader.size()/1000);
		socket->Send((BYTE*)fileHeader.data(), fileHeader.size());
	}

	return fileSize - fileHeader.size();

}

void BarbaCourierHttpClient::WaitForAcceptPostRequest(BarbaSocket* barbaSocket)
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
			Log3("Receiving from POST-REPLY payload. Payload: %d bytes.", contentLength);
		}
	}
	catch (...)
	{
		barbaSocket->SetReceiveTimeOut(oldTimeOut);
		throw;
	}
}


size_t BarbaCourierHttpClient::SendGetRequest(BarbaSocket* socket)
{
	TCHAR filename[MAX_PATH];

	size_t fileSize = 0;
	std::tstring contentType;
	GetFakeFile(filename, &contentType, &fileSize, NULL, true);

	std::tstring httpRequest = GetHttpGetTemplate(false);
	std::tstring requestData = CreateRequestString(false, fileSize, 0);
	BarbaUtils::UpdateHttpRequest(&httpRequest, GetCreateStruct()->HostName.data(), filename, contentType.data(), 0, requestData.data());

	//Log
	if (!GetRequestMode()->BombardGet)
		Log2(_T("Sending HTTP GET request! FileName: %s."), filename);

	//send request
	socket->Send((BYTE*)httpRequest.data(), httpRequest.size());
	return fileSize;
}

void BarbaCourierHttpClient::SendGetRequestBombard(BarbaSocket* socket, BarbaBuffer* content)
{
	TCHAR filename[MAX_PATH];

	std::tstring contentType;
	GetFakeFile(filename, &contentType, NULL, NULL, true);

	std::tstring httpRequest = GetHttpGetTemplate(true);
	BarbaUtils::UpdateHttpRequest(&httpRequest, GetCreateStruct()->HostName.data(), filename, contentType.data(), content->size(), NULL);

	//send buffer
	BarbaBuffer buffer;
	buffer.reserve(httpRequest.length() + content->size());
	buffer.append((char*)httpRequest.data(), httpRequest.size());
	buffer.append(content);

	//send request
	Log3(_T("Sending HTTP Bombard GET request. Payload: %d bytes."), content->size());
	socket->Send(buffer.data(), buffer.size());
}

void BarbaCourierHttpClient::BeforeSendMessage(BarbaSocket* /*barbaSocket*/, BarbaBuffer* messageBuffer)
{
	if (GetRequestMode()->BombardPost)
	{
		BarbaBuffer buffer;
		GetPostRequestBombard(messageBuffer->size(), &buffer);
		buffer.append(messageBuffer);
		messageBuffer->assign(&buffer);
		Log3(_T("Sending with POST request. Count: %d bytes."), messageBuffer->size());
	}
	else
	{
		Log3(_T("Sending to POST channel. Count: %d bytes."), messageBuffer->size());
	}
}

void BarbaCourierHttpClient::AfterSendMessage(BarbaSocket* barbaSocket, bool isTransferFinished)
{
	if (GetRequestMode()->BombardPostReply && !isTransferFinished)
	{
		WaitForAcceptPostRequest(barbaSocket);
	}
}

void BarbaCourierHttpClient::BeforeReceiveMessage(BarbaSocket* barbaSocket, size_t* chunkSize)
{
	if (GetRequestMode()->BombardGet)
	{
		std::string header = barbaSocket->ReadHttpRequest();
		if (header.empty())
			throw new BarbaException( _T("Server does not accept request!") );
		*chunkSize = BarbaUtils::GetKeyValueFromString(header.data(), _T("Content-Length"), 0);
		if (*chunkSize>0)
			Log3("Receiving from GET request. Count: %d bytes.", *chunkSize);
	}
}


void BarbaCourierHttpClient::AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength, bool isTransferFinished)
{
	if (GetRequestMode()->BombardGet && !isTransferFinished)
	{

		BarbaArray<Message*> messages;
		try
		{
			//attach some message to get request
			BarbaBuffer buffer;
			if (GetRequestMode()->BombardGetPayload)
			{
				GetMessages(messages);
				ProcessOutgoingMessages(messages, 0, 0, &buffer);
				if (buffer.size()>0)
					Log3("Sending with GET Payload. Payload: %d bytes.", buffer.size());
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
		Log3("Receiving from GET channel. Count: %d bytes.", messageLength);
	}
}

void BarbaCourierHttpClient::WaitForIncomingFileHeader(BarbaSocket* socket, size_t fileHeaderSize)
{
	if (fileHeaderSize==0)
	{
		Log2(_T("Request does not have file header."));
	} 
	else if (fileHeaderSize>BarbaCourier_MaxFileHeaderSize)
	{
		throw new BarbaException(_T("Fake header could not be more than %u size! Requested Size: %u."), BarbaCourier_MaxFileHeaderSize, fileHeaderSize);
	}
	else
	{
		Log2(_T("Waiting for incoming file header. HeaderSize: %u KB."), fileHeaderSize/1000);

		BarbaBuffer buffer(fileHeaderSize);
		if (socket->Receive(buffer.data(), buffer.size(), true)!=(int)buffer.size())
			throw new BarbaException(_T("Could not receive file header."));
	}
}

void BarbaCourierHttpClient::ClientWorker(ClientWorkerData* clientWorkerData)
{
	bool hasError = false;
	bool isOutgoing = clientWorkerData->IsOutgoing;
	DWORD retryTime = 10000;

	while (!IsDisposing())
	{
		BarbaSocketClient* socket = NULL;

		try
		{
			hasError = false;

			//create socket
			u_short port = GetCreateStruct()->PortRange->GetRandomPort();
			Log2(_T("Opening TCP Connection for HTTP %s connection. RemotePort: %d."),  isOutgoing ? _T("POST") : _T("GET"), port);
			socket = new BarbaSocketClient(GetCreateStruct()->RemoteIp, port);

			//add socket to store
			Sockets_Add(socket, isOutgoing);

			if (isOutgoing)
			{
				if (GetRequestMode()->BombardPost || GetRequestMode()->BombardPostReply)
				{
					//report new connection
					Log2(_T("HTTP Bombard %s added. Connections Count: %d."), GetRequestMode()->BombardPostReply ? _T("POST & REPLY") : _T("POST"), OutgoingSockets.GetCount());
					retryTime = 3000;

					//send initial post request
					size_t fileSize = SendPostRequest(socket, true);

					//Wait to accept initial bombard post request
					Log2(_T("Waiting for server to accept HTTP POST request."));
					WaitForAcceptPostRequest(socket);

					//process socket until socket closed
					ProcessOutgoing(socket, fileSize);

					//finish TCP connection
					Log2(_T("HTTP Bombard POST transfer completed."));

				}
				else
				{
					//report new connection
					Log2(_T("HTTP POST added. Connections Count: %d."), OutgoingSockets.GetCount());
					size_t fileSize = SendPostRequest(socket, false);

					//process socket until socket closed or finish uploading fakeFileSize
					ProcessOutgoing(socket, fileSize);

					//wait for accept post request
					WaitForAcceptPostRequest(socket);
					Log2(_T("HTTP POST file completed."));
				}
			}
			else
			{
				Log2(_T("HTTP %s added. Connections Count: %d."), GetRequestMode()->BombardGet ? _T("Bombard GET") : _T("GET"), IncomingSockets.GetCount());

				//send GET request
				size_t fileSize = SendGetRequest(socket);

				if (GetRequestMode()->BombardGet)
				{
					retryTime = 3000;

					//process socket until socket closed
					ProcessIncoming(socket, fileSize);

					//finish current connection
					Log2(_T("HTTP Bombard GET Transfer Completed."));
				}
				else
				{
					//wait for reply
					Log2(_T("Waiting for server to accept HTTP GET request."));
					std::string httpReply = socket->ReadHttpRequest();
					if (httpReply.empty())
						throw new BarbaException( _T("Server does not reply to HTTP GET request!") );

					//process reply
					std::tstring requestData = RequestData_FromString(httpReply);
					if (requestData.empty())
						throw new BarbaException(_T("Server does not return requestData in its HTTP GET Request reply!"));
					size_t fileHeaderSize = BarbaUtils::GetKeyValueFromString(requestData.data(), _T("FileHeaderSize"), 0);

					//wait for incoming file header
					Log2(_T("Downloading file %u KB. HeaderSize: %u KB"), fileSize/1000, fileHeaderSize/1000);
					WaitForIncomingFileHeader(socket, fileHeaderSize);

					//process socket until socket closed
					ProcessIncoming(socket, fileSize-fileHeaderSize);

					//report
					Log2(_T("HTTP GET file completed."));
				}
			}
		}
		catch (BarbaException* er)
		{
			hasError = true;
			Log2(_T("Error: %s"), er->ToString());
			delete er;
		}

		//delete socket
		if (socket!=NULL)
		{
			//remove socket from store
			Sockets_Remove(socket, isOutgoing);
			socket = NULL;
			Log2(_T("HTTP %s connection removed. Connections Count: %d."), isOutgoing ? _T("POST") : _T("GET"), isOutgoing ? OutgoingSockets.GetCount() : IncomingSockets.GetCount());
		}

		//wait for next connection if isProccessed not set;
		//if isProccessed not set it mean server reject the connection so wait 5 second
		//if isProccessed is set it mean connection data transfer has been finished and new connection should ne established as soon as possible
		if (hasError)
		{
			Log2(_T("Retrying in %d second..."), retryTime/1000);
			DisposeEvent.Wait(retryTime);
		}
	}
}
