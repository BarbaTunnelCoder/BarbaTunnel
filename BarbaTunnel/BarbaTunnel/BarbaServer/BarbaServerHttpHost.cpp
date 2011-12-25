#include "StdAfx.h"
#include "BarbaServerHttpHost.h"
#include "BarbaServerHttpConnection.h"
#include "BarbaServerApp.h"


BarbaServerHttpHost::BarbaServerHttpHost(void)
	: DisposeEvent(true, false)
{
}


BarbaServerHttpHost::~BarbaServerHttpHost(void)
{
}

void BarbaServerHttpHost::Dispose()
{
	//signal disposing
	DisposeEvent.Set();

	//close listener sockets
	BarbaApp::CloseSocketsList(&this->ListenerSockets);
	//close answer sockets
	BarbaApp::CloseSocketsList(&this->AnswerSockets);

	//wait for listener threads to finish
	HANDLE threadHandle = this->ListenerThreads.RemoveHead();
	while (threadHandle!=NULL)
	{
		WaitForSingleObject(threadHandle, INFINITE);
		CloseHandle(threadHandle);
		threadHandle = this->ListenerThreads.RemoveHead();
	}

	//wait for answer threads to finish
	threadHandle = this->AnswerThreads.RemoveHead();
	while (threadHandle!=NULL)
	{
		WaitForSingleObject(threadHandle, INFINITE);
		CloseHandle(threadHandle);
		threadHandle = this->AnswerThreads.RemoveHead();
	}
}


bool BarbaServerHttpHost::IsDisposing()
{
	return DisposeEvent.Wait(0)==WAIT_OBJECT_0;
}

unsigned int BarbaServerHttpHost::AnswerThread(void* data)
{
	bool success = false;
	AnswerThreadData* threadData = (AnswerThreadData*)data;
	BarbaServerHttpHost* _this = (BarbaServerHttpHost*)threadData->HttpHost;
	BarbaSocket* socket = (BarbaSocket*)threadData->Socket;

	//get clientIp
	TCHAR clientIpString[30];
	PacketHelper::ConvertIpToString(socket->GetRemoteIp(), clientIpString, _countof(clientIpString));

	try
	{
	//read httpRequest
		BarbaLog2(_T("HTTP Host: TID: %x, New incoming connection. ServerPort: %d, ClientIp: %s."), GetCurrentThreadId(), threadData->ServerPort, clientIpString);
		BarbaLog2(_T("HTTP Host: Waiting for HTTP request."));
		std::string httpRequest = socket->ReadHttpRequest();
		bool isGet = httpRequest.size()>=3 && _strnicmp(httpRequest.data(), "GET", 3)==0;
		bool isPost = httpRequest.size()>=4 && _strnicmp(httpRequest.data(), "POST", 4)==0;
		if (isGet || isPost)
		{
			//find session
			bool isOutgoing = isGet;
			std::tstring sessionIdStr = BarbaUtils::GetKeyValueFromHttpRequest(httpRequest.data(), threadData->ConfigItem->SessionKeyName.data());
			u_long sessionId = strtoul(sessionIdStr.data(), NULL, 32);
			if (sessionId==0)
				new BarbaException( _T("Could not extract sessionId from HTTP request!") );

			//find connection by session id
			SimpleLock lock(&_this->CreateConnectionCriticalSection);
			BarbaServerHttpConnection* conn = (BarbaServerHttpConnection*)theServerApp->ConnectionManager.FindBySessionId(sessionId);
			//create new connection if session not found
			if (conn==NULL)
				conn = theServerApp->ConnectionManager.CreateHttpConnection(threadData->ConfigItem, socket->GetRemoteIp(), threadData->ServerPort, sessionId);
			lock.Unlock();

			//add socket to HTTP connection
			if (conn!=NULL)
				success = conn->AddSocket(socket, httpRequest.data(), isOutgoing);
		}
	}
	catch(BarbaException* er)
	{
		BarbaLog2(_T("HTTP Host: TID: %x, Error: %s"), GetCurrentThreadId(), er->ToString());
		delete er;
	}
	catch(...)
	{
		BarbaLog2(_T("HTTP Host: TID: %x, Unknown Error!"), GetCurrentThreadId());
	}

	_this->AnswerSockets.Remove(socket);
	if (!success) delete socket;
	delete threadData;
	return success ? 0 : 1;
}


unsigned int BarbaServerHttpHost::ListenerThread(void* data)
{
	ListenerThreadData* threadData = (ListenerThreadData*)data;
	BarbaServerHttpHost* _this = (BarbaServerHttpHost*)threadData->HttpHost;
	BarbaSocketServer* listenerSocket = (BarbaSocketServer*)threadData->SocketServer;

	try
	{
		while (!_this->IsDisposing())
		{
			BarbaSocket* socket = listenerSocket->Accept();
			_this->AnswerSockets.AddTail(socket);
			AnswerThreadData* answerThreadData = new AnswerThreadData(_this, socket, threadData->ConfigItem, listenerSocket->GetListenPort());
			_this->AnswerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, AnswerThread, answerThreadData, 0, NULL));
			BarbaServerApp::CloseFinishedThreadHandle(&_this->AnswerThreads);
		}
	}
	catch(BarbaException* er)
	{
		BarbaLog2(_T("HttpHost: %s"), er->ToString());
		delete er;
	}
	catch(...)
	{
		BarbaLog2(_T("HTTP Host: Unknown Error!"));
	}

	_this->ListenerSockets.Remove(listenerSocket);
	BarbaLog2(_T("HttpHost: TCP listener closed, port: %d."), listenerSocket->GetListenPort());
	delete listenerSocket;
	delete threadData;
	return 0;
}

void BarbaServerHttpHost::AddListenerPort(BarbaServerConfigItem* configItem, u_short port)
{
	BarbaSocketServer* listenSocket = new BarbaSocketServer(port);
	ListenerSockets.AddTail(listenSocket);
	ListenerThreadData* threadData = new ListenerThreadData(this, listenSocket, configItem);
	ListenerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, ListenerThread, threadData, 0, NULL));
}

void BarbaServerHttpHost::Initialize()
{
	//Initialize listeners
	int createdSocket = 0;
	for (size_t i=0; i<theServerApp->Config.ItemsCount; i++)
	{
		BarbaServerConfigItem* item = &theServerApp->Config.Items[i];
		if (item->Mode!=BarbaModeHttpTunnel)
			continue;

		//Initialize HttpHost to listen to ports
		for (size_t j=0; j<item->TunnelPortsCount; j++)
		{
			PortRange* portRange = &item->TunnelPorts[j];
			for (u_short port=portRange->StartPort; port<=portRange->EndPort; port++)
			{
				//check added port count
				if (createdSocket>=BARBA_MAX_SERVERLISTENSOCKET)
				{
					BarbaLog(_T("Error: HTTP Tunnel could not listen more than %d ports!"), BARBA_MAX_SERVERLISTENSOCKET);
					j = item->TunnelPortsCount;
					break;
				}

				//add port
				try
				{
					BarbaLog2(_T("HTTP Host: Listening to TCP port %d."), portRange->StartPort);
					this->AddListenerPort(item, port);
					createdSocket++;
				}
				catch (...)
				{
					BarbaLog(_T("Error! HTTP Host could not listen to TCP port %d!"), portRange->StartPort);
				}
			}
		}// for j
	}
}