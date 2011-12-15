/*
 * BarbaServerHttpHost class
 * this class just used for HTTP-Tunneling
 */

#pragma once
#include "General.h"
#include "BarbaServerHttpCourier.h"

class BarbaServerHttpHost
{
private:
	//used to pass data to created thread
	struct ListenerThreadData
	{
		ListenerThreadData(BarbaServerHttpHost* httpServer, BarbaSocketServer* socketServer) { this->HttpServer=httpServer; this->SocketServer=socketServer; }
		BarbaServerHttpHost* HttpServer;
		BarbaSocketServer* SocketServer;
	};

	//used to pass data to created thread
	struct AnswerThreadData
	{
		AnswerThreadData(BarbaServerHttpHost* httpServer, BarbaSocket* socket) { this->HttpServer=httpServer; this->Socket=socket; }
		BarbaServerHttpHost* HttpServer;
		BarbaSocket* Socket;
	};

public:
	explicit BarbaServerHttpHost();
	virtual ~BarbaServerHttpHost(void);

	//@throw 
	void AddListenerPort(u_short port);
protected:

private:
	virtual void Dispose();
	bool IsDisposing();
	SimpleEvent DisposeEvent;
	static void CloseFinishedThreadHandle(SimpleSafeList<HANDLE>* list);

	static u_long ExtractSessionId(LPCSTR header);
	static unsigned int __stdcall AnswerThread(void* answerThreadData);
	static unsigned int __stdcall ListenerThread(void* listenerThreadData);
	SimpleSafeList<BarbaSocketServer*> ListenerSockets;
	SimpleSafeList<HANDLE> ListenerThreads;
	SimpleSafeList<HANDLE> AnswerThreads;


};

