#include "StdAfx.h"
#include "BarbaHttpServer.h"
#include "BarbaServerApp.h"


BarbaHttpServer::BarbaHttpServer(void)
	: DisposeEvent(true, false)
{
}


BarbaHttpServer::~BarbaHttpServer(void)
{
	Dispose();
}

void BarbaHttpServer::Dispose()
{
	//signal disposing
	DisposeEvent.Set();

	//close listener sockets
	size_t count = this->ListenerSockets.GetCount();
	BarbaSocketServer** socketsArray = new BarbaSocketServer*[count];
	this->ListenerSockets.GetAll(socketsArray, &count);
	for (size_t i=0; i<count; i++)
	{
		try
		{
			socketsArray[i]->Close();
		}
		catch(...)
		{
		}
	}
	delete socketsArray;

	//wait for all thread to finish
	HANDLE threadHandle = this->ListenerThreads.RemoveHead();
	while (threadHandle!=NULL)
	{
		WaitForSingleObject(threadHandle, INFINITE);
		CloseHandle(threadHandle);
		threadHandle = this->ListenerThreads.RemoveHead();
	}
}


bool BarbaHttpServer::IsDisposing()
{
	return DisposeEvent.Wait(0)==WAIT_OBJECT_0;
}


unsigned int BarbaHttpServer::ListenerThread(void* data)
{
	ListenerThreadData* threadData = (ListenerThreadData*)data;
	BarbaHttpServer* _this = (BarbaHttpServer*)threadData->HttpServer;
	BarbaSocketServer* listenerSocket = (BarbaSocketServer*)threadData->SocketServer;

	try
	{
		while (!_this->IsDisposing())
		{
			BarbaSocket* socket = listenerSocket->Accept();
			try
			{
				//std::string header = socket->ReadHttpHeader();
				//if (!header.empty())
				//{
					//find session
					//if (!myServer->AddSocket(s, false))
						//delete s;
				//}
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

void BarbaHttpServer::AddListenerPort(u_short port)
{
	BarbaSocketServer* listenSocket = new BarbaSocketServer(port);
	ListenerSockets.AddTail(listenSocket);
	ListenerThreadData* threadData = new ListenerThreadData(this, listenSocket);
	ListenerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, ListenerThread, threadData, 0, NULL));
}

/*
unsigned int BarbaHttpServer::SocketAnswerThread(void* data)
{
	SocketListnerThreadData* threadData = (SocketListnerThreadData*)data;
	BarbaServerApp* _this = (BarbaServerApp*)threadData->Socket;
	BarbaSocket* socket = (BarbaSocketServer*)threadData->Socket;

	//read header
	std::string header = socket->ReadHttpHeader();

	//find session
	//if (!myServer->AddSocket(socket, false))
	{
		_this->ServerSockets.Remove(socket);
		delete socket;
	}
}
*/