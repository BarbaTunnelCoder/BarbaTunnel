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
	SimpleSafeList<BarbaSocketServer*>::AutoLockBuffer autoLockBuf(&this->ListenerSockets);
	BarbaSocketServer** socketsArray = autoLockBuf.GetBuffer();
	for (size_t i=0; i<this->ListenerSockets.GetCount(); i++)
	{
		try
		{
			socketsArray[i]->Close();
		}
		catch(...)
		{
		}
	}
	autoLockBuf.Unlock();

	//close answering sockets


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
	AnswerThreadData* threadData = (AnswerThreadData*)data;
	//BarbaServerHttpHost* _this = (BarbaServerHttpHost*)threadData->HttpServer;
	BarbaSocket* socket = (BarbaSocket*)threadData->Socket;

	//read header
	try
	{
		std::string header = socket->ReadHttpHeader();
		if (!header.empty())
		{
			//find session
			bool isOutgoing = true; //ToDo
			u_long sessionId = ExtractSessionId(header.data());
			if (sessionId==0)
				throw _T("Could not find sessionId in header!");

			//find connection by session id
			BarbaServerHttpConnection* conn = (BarbaServerHttpConnection*)theServerApp->ConnectionManager.FindBySessionId(sessionId);

			//create new connection if session not found
			if (conn==NULL)
				theServerApp->ConnectionManager.CreateHttpConnection(threadData->ConfigItem, socket->GetRemoteIp(), threadData->ServerPort, sessionId);

			//add socket to http connection
			if (conn!=NULL)
			{
				conn->AddSocket(socket, isOutgoing);
				return 0;
			}
		}
	}
	catch(...)
	{
	}

	socket->Close();
	delete socket;
	return 1;
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
			try
			{
				AnswerThreadData* answerThreadData = new AnswerThreadData(_this, socket, threadData->ConfigItem, listenerSocket->GetListenPort());
				_this->AnswerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, AnswerThread, answerThreadData, 0, NULL));
				CloseFinishedThreadHandle(&_this->AnswerThreads);
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
	return 0;
}

void BarbaServerHttpHost::AddListenerPort(BarbaServerConfigItem* configItem, u_short port)
{
	BarbaSocketServer* listenSocket = new BarbaSocketServer(port);
	ListenerSockets.AddTail(listenSocket);
	ListenerThreadData* threadData = new ListenerThreadData(this, listenSocket, configItem);
	ListenerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, ListenerThread, threadData, 0, NULL));
}

void BarbaServerHttpHost::CloseFinishedThreadHandle(SimpleSafeList<HANDLE>* list)
{
	SimpleSafeList<HANDLE>::AutoLockBuffer autoLockBuf(list);
	HANDLE* handles = autoLockBuf.GetBuffer();
	for (size_t i=0; i<list->GetCount(); i++)
	{
		bool alive = false;
		if ( BarbaUtils::IsThreadAlive(handles[i], &alive) && !alive)
		{
			list->Remove(handles[i]);
			CloseHandle(handles[i]);
		}
	}
}

u_long BarbaServerHttpHost::ExtractSessionId(LPCSTR header)
{
	const char* key = "session=";
	const char* start = strstr(header, key);
	if (start==NULL)
		return 0;
	start = start + strlen(key);

	const char* end = strstr(start, ";");
	if (end==NULL)
		end = header + strlen(header);

	char sessionBuffer[100];
	strncpy_s(sessionBuffer, start, end-start);
	return strtoul(sessionBuffer, 0, 10);
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