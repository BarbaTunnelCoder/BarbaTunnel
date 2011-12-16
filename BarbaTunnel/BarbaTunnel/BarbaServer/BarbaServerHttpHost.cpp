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

	//wait for all thread to finish
	HANDLE threadHandle = this->ListenerThreads.RemoveHead();
	while (threadHandle!=NULL)
	{
		WaitForSingleObject(threadHandle, INFINITE);
		CloseHandle(threadHandle);
		threadHandle = this->ListenerThreads.RemoveHead();
	}
}


bool BarbaServerHttpHost::IsDisposing()
{
	return DisposeEvent.Wait(0)==WAIT_OBJECT_0;
}

unsigned int BarbaServerHttpHost::AnswerThread(void* data)
{
	AnswerThreadData* threadData = (AnswerThreadData*)data;
	BarbaServerHttpHost* _this = (BarbaServerHttpHost*)threadData->HttpServer;
	BarbaSocket* socket = (BarbaSocket*)threadData->Socket;

	//read header
	try
	{
		std::string header = socket->ReadHttpHeader();
		if (!header.empty())
		{
			//find session
			bool isOutgoing = true;
			u_long sessionId = ExtractSessionId(header.data());
			if (sessionId==0)
				throw _T("Could not find sessionId in header!");

			//find connection by session id
			BarbaServerHttpConnection* conn = (BarbaServerHttpConnection*)theServerApp->ConnectionManager.FindBySessionId(sessionId);

			//create new connection if session not found
			if (conn==NULL)
				conn=conn;

			//check connection mode
			if (conn->GetMode()!=BarbaModeHttpTunnel)
				throw _T("Invalid mode for connection!");

			if (conn->AddSocket(socket, isOutgoing))
				return 0;
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
				AnswerThreadData* threadData = new AnswerThreadData(_this, socket);
				_this->AnswerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, AnswerThread, threadData, 0, NULL));
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

void BarbaServerHttpHost::AddListenerPort(u_short port)
{
	BarbaSocketServer* listenSocket = new BarbaSocketServer(port);
	ListenerSockets.AddTail(listenSocket);
	ListenerThreadData* threadData = new ListenerThreadData(this, listenSocket);
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
