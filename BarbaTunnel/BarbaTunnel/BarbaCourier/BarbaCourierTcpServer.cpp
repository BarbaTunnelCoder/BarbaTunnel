#include "StdAfx.h"
#include "BarbaCourierTcpServer.h"
#include "BarbaUtils.h"

BarbaCourierTcpServer::BarbaCourierTcpServer(CreateStrcutTcp* cs)
	: BarbaCourier(cs)
{
}

void BarbaCourierTcpServer::AddSocket(BarbaSocket* barbaSocket, LPCTSTR requestString, LPCTSTR requestData)
{
	if (this->IsDisposing())
		throw new BarbaException(_T("Could not add to disposing object!"));

	//Add Socket
	bool isOutgoing = BarbaUtils::GetKeyValueFromString(requestData, _T("Outgoing"), 0)==0;
	Sockets_Add(barbaSocket, isOutgoing);

	//start threads
	ServerWorkerData* workerData = new ServerWorkerData();
	workerData->Courier = this;
	workerData->Socket = barbaSocket;
	workerData->RequestData = requestData;
	workerData->RequestString = requestString;
	
	//start thread
	Threads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, ServerWorkerThread, workerData, 0, NULL) );
}

unsigned int BarbaCourierTcpServer::ServerWorkerThread(void* serverWorkerData)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	ServerWorkerData* workerData = (ServerWorkerData*)serverWorkerData;
	BarbaCourierTcpServer* _this = (BarbaCourierTcpServer*)workerData->Courier;

	//start worker
	_this->ServerWorker(workerData);
	
	//clean up
	delete workerData;
	return 0;
}


BarbaCourierTcpServer::~BarbaCourierTcpServer(void)
{
}

void BarbaCourierTcpServer::Init(LPCTSTR requestData)
{
	GetCreateStruct()->MinPacketSize = BarbaUtils::GetKeyValueFromString(requestData, _T("MinPacketSize"), 0);
	GetCreateStruct()->KeepAliveInterval = BarbaUtils::GetKeyValueFromString(requestData, _T("KeepAliveInterval"), 0);
	GetCreateStruct()->SessionId = BarbaUtils::GetKeyValueFromString(requestData, _T("SessionId"), 0);
}

void BarbaCourierTcpServer::ServerWorker(ServerWorkerData* workerData)
{
	BarbaSocket* socket = workerData->Socket;
	bool isOutgoing = BarbaUtils::GetKeyValueFromString(workerData->RequestData.data(), _T("Outgoing"), 0)==0;
	size_t transferSize = BarbaUtils::GetKeyValueFromString(workerData->RequestData.data(), _T("TransferSize"), 0);

	try
	{
		if (isOutgoing)
		{
			//report new connection
			Log2(_T("TCP SEND channel added. Port: %d, TransferSize: %d KB, Connections Count: %d."), socket->GetLocalPort(), transferSize/1000, OutgoingSockets.GetCount());

			//process socket until socket closed or or file transfered
			ProcessOutgoing(socket, transferSize);

			//completed
			Log2(_T("TCP SEND channel completed."));
		}
		else
		{
			//report new connection
			Log2(_T("TCP RECIEVE channel added. Port: %d, TransferSize: %d KB, Connections Count: %d."), socket->GetLocalPort(), transferSize/1000, IncomingSockets.GetCount());

			//process socket until socket closed or file transfered
			ProcessIncoming(socket, transferSize);

			//completed
			Log2(_T("TCP RECIEVE channel completed."));
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
	Log2(_T("TCP %s connection removed. Connections Count: %d."), isOutgoing ? _T("SEND") : _T("RECEIVE"), isOutgoing ? OutgoingSockets.GetCount() : IncomingSockets.GetCount());
}
