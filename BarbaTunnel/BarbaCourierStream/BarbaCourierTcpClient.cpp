#include "StdAfx.h"
#include "BarbaCourierTcpClient.h"
#include "BarbaUtils.h"


BarbaCourierTcpClient::BarbaCourierTcpClient(CreateStrcutTcp* cs)
	: BarbaCourierStream(cs)
{
}

BarbaCourierTcpClient::~BarbaCourierTcpClient()
{
}

void BarbaCourierTcpClient::Init()
{
	BarbaCourierStream::Init();
	StartClientThreads();
}


void BarbaCourierTcpClient::StartClientThreads()
{
	for (u_short i=0; i<GetCreateStruct()->MaxConnections; i++)
	{
		//create outgoing connection thread (POST)
		ClientWorkerData* outgoingThreadData = new ClientWorkerData;
		outgoingThreadData->Courier = this;
		outgoingThreadData->IsOutgoing = true;
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, ClientWorkerThread, outgoingThreadData, 0, NULL));

		//create incoming connection thread (GET)
		ClientWorkerData* incomingThreadData = new ClientWorkerData;
		incomingThreadData->Courier = this;
		incomingThreadData->IsOutgoing = false;
		Threads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, ClientWorkerThread, incomingThreadData, 0, NULL));
	}

	//KeepAlive usefull for client, because server always accept new connection and close old one but client will block when all of its connection remain open without end-point
	StartKeepAliveThread();

}

std::tstring BarbaCourierTcpClient::CreateRequestString(bool outgoing, size_t transferSize, LPCTSTR other)
{
	std::tstring requestData;
	BarbaUtils::SetKeyValue(&requestData, _T("SessionId"), GetCreateStruct()->SessionId);
	BarbaUtils::SetKeyValue(&requestData, _T("MinPacketSize"), (int)GetCreateStruct()->MinPacketSize);
	BarbaUtils::SetKeyValue(&requestData, _T("KeepAliveInterval"), (int)GetCreateStruct()->KeepAliveInterval);
	BarbaUtils::SetKeyValue(&requestData, _T("TransferSize"), (int)transferSize);
	BarbaUtils::SetKeyValue(&requestData, _T("Outgoing"), outgoing);
	if (other!=NULL) requestData.append(other);
	return RequestData_ToString(requestData);
}

unsigned int BarbaCourierTcpClient::ClientWorkerThread(void* clientWorkerThreadData)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	ClientWorkerData* workerData = (ClientWorkerData*)clientWorkerThreadData;
	BarbaCourierTcpClient* _this = workerData->Courier;
	
	//start worker
	_this->ClientWorker(workerData);
	
	//clean up
	delete clientWorkerThreadData;
	return 0;
}

void BarbaCourierTcpClient::ClientWorker(ClientWorkerData* clientWorkerData)
{
	bool isOutgoing = clientWorkerData->IsOutgoing;
	bool hasError = false;
	DWORD retryTime = 5000;

	while (!IsDisposing())
	{
		BarbaSocketClient* socket = NULL;

		try
		{
			hasError = false;

			//create socket
			u_short port = GetCreateStruct()->PortRange->GetRandomPort();
			Log2(_T("Opening TCP Connection for %s channel. RemotePort: %d."),  isOutgoing ? _T("SEND") : _T("RECEIVE"), port);
			socket = new BarbaSocketClient(GetCreateStruct()->RemoteIp, port);
			Sockets_Add(socket, isOutgoing);

			//prepare equest
			size_t transferSize = BarbaUtils::GetRandom((u_int)GetCreateStruct()->MaxTransferSize/2, (u_int)GetCreateStruct()->MaxTransferSize); 
			std::string requestString = CreateRequestString(isOutgoing, transferSize);
			requestString = BarbaUtils::PrepareHttpRequest(requestString);

			if (isOutgoing)
			{
					//report new connection
					Log2(_T("TCP SEND channel added. Port: %d, TransferSize: %d KB, Connections Count: %d."), socket->GetRemotePort(), transferSize/1000, OutgoingSockets.GetCount());

					//send request
					Log3(_T("Sending TCP SEND request..."));
					socket->Send((BYTE*)requestString.data(), requestString.size());

					//process socket until socket closed or transfer completed
					ProcessOutgoing(socket, GetCreateStruct()->MaxTransferSize);

					Log2(_T("TCP SEND channel completed."));
			}
			else
			{
				Log2(_T("TCP RECEIVE channel added. Port: %d, TransferSize: %d KB, Connections Count: %d."), socket->GetRemotePort(), transferSize/1000, IncomingSockets.GetCount());

				//process socket until socket closed or transfer completed
				Log3(_T("Sending TCP RECEIVE request..."));
				socket->Send((BYTE*)requestString.data(), requestString.size());

				//process socket until socket closed or transfer completed
				ProcessIncoming(socket, transferSize);

				//report
				Log2(_T("TCP RECEIVE channel completed."));
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
			Log2(_T("TCP %s connection removed. Connections Count: %d."), isOutgoing ? _T("SEND") : _T("RECEIVE"), isOutgoing ? OutgoingSockets.GetCount() : IncomingSockets.GetCount());
		}

		//wait for next connection if isProccessed not set;
		//if isProccessed not set it mean server reject the connection so wait some second
		//if isProccessed is set it mean connection data transfer has been finished and new connection should ne established as soon as possible
		if (hasError)
		{
			Log2(_T("Retrying in %d second..."), retryTime/1000);
			DisposeEvent.Wait(retryTime);
		}
	}
}

