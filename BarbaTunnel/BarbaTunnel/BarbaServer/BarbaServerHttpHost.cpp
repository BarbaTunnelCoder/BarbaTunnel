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
	Dispose();
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
	BarbaServerHttpHost* _this = (BarbaServerHttpHost*)threadData->HttpServer;
	BarbaSocket* socket = (BarbaSocket*)threadData->Socket;

	//read header
	try
	{
		std::string header = socket->ReadHttpHeader();
		bool isGet = header.size()>=3 && _strnicmp(header.data(), "GET", 3)==0;
		bool isPost = header.size()>=4 && _strnicmp(header.data(), "POST", 4)==0;
		if (isGet || isPost)
		{
			//find session
			bool isOutgoing = isGet;
			printf("Answer\n");
			u_long sessionId = ExtractSessionId(header.data());
			printf("Answer %u\n", sessionId);
			if (sessionId==0)
				throw _T("Could not find sessionId in header!");

			//find connection by session id
			printf("Try to create\n");
			SimpleLock lock(&_this->CreateConnectionCriticalSection);
			BarbaServerHttpConnection* conn = (BarbaServerHttpConnection*)theServerApp->ConnectionManager.FindBySessionId(sessionId);

			//create new connection if session not found
			if (conn==NULL)
				conn = theServerApp->ConnectionManager.CreateHttpConnection(threadData->ConfigItem, socket->GetRemoteIp(), threadData->ServerPort, sessionId);
			lock.Unlock();

			//add socket to http connection
			if (conn!=NULL)
				success = conn->AddSocket(socket, isOutgoing);
			if (!success)
				printf("Failed to add!\n");
		}
	}
	catch(...)
	{
	}

	_this->AnswerSockets.Remove(socket);
	if (!success) delete socket;
	delete threadData;
	return success ? 0 : 1;
}


unsigned int BarbaServerHttpHost::ListenerThread(void* data)
{
	ListenerThreadData* threadData = (ListenerThreadData*)data;
	BarbaServerHttpHost* _this = (BarbaServerHttpHost*)threadData->HttpServer;
	BarbaSocketServer* listenerSocket = (BarbaSocketServer*)threadData->SocketServer;

	try
	{
		while (!_this->IsDisposing())
		{
			BarbaSocket* socket = listenerSocket->Accept();
			_this->AnswerSockets.AddTail(socket);
			try
			{
				AnswerThreadData* answerThreadData = new AnswerThreadData(_this, socket, threadData->ConfigItem, listenerSocket->GetListenPort());
				_this->AnswerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, AnswerThread, answerThreadData, 0, NULL));
				BarbaServerApp::CloseFinishedThreadHandle(&_this->AnswerThreads);
			}
			catch(...)
			{
			}
		}
	}
	catch(...)
	{
	}

	_this->ListenerSockets.Remove(listenerSocket);
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

u_long BarbaServerHttpHost::ExtractSessionId(LPCSTR header)
{
	CHAR key[BARBA_MaxKeyName+2];
	sprintf_s(key, "%s=", theServerApp->Config.SessionKeyName, key);
	const CHAR* start = strstr(header, key);
	if (start==NULL)
		return 0;
	start = start + strlen(key);

	const CHAR* end = strstr(start, ";");
	if (end==NULL) end = strstr(start, "\r");
	if (end==NULL) end = header + strlen(header);

	CHAR sessionBuffer[100];
	strncpy_s(sessionBuffer, start, end-start);
	return strtoul(sessionBuffer, 0, 16);
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

		//Initialize HttpServer to listen to ports
		for (size_t j=0; j<item->TunnelPortsCount; j++)
		{
			PortRange* portRange = &item->TunnelPorts[j];
			for (u_short port=portRange->StartPort; port<=portRange->EndPort; port++)
			{
				//check added port count
				if (createdSocket>=BARBA_MAX_SERVERLISTENSOCKET)
				{
					BarbaLog(_T("BarbaServer could not listen more than %d ports!"), BARBA_MAX_SERVERLISTENSOCKET);
					j = item->TunnelPortsCount;
					break;
				}

				//add port
				try
				{
					this->AddListenerPort(item, port);
					createdSocket++;
				}
				catch (...)
				{
					BarbaLog(_T("Error: BarbaServer could not listen to TCP port %d!"), portRange->StartPort);
				}
			}
		}// for j
	}
}