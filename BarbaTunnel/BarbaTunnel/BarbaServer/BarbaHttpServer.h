/*
 * BarbaHttpServer class
 * this class just used for HTTP-Tunneling
 */

#pragma once
#include "General.h"
#include "BarbaHttpServerCourier.h"

class BarbaHttpServer
{
private:
	//used to pass data to created thread
	struct ListenerThreadData
	{
		ListenerThreadData(BarbaHttpServer* httpServer, BarbaSocketServer* socketServer) { this->HttpServer=httpServer; this->SocketServer=socketServer; }
		BarbaHttpServer* HttpServer;
		BarbaSocketServer* SocketServer;
	};

public:
	BarbaHttpServer(void);
	virtual ~BarbaHttpServer(void);


	//@throw 
	void AddListenerPort(u_short port);

protected:

private:
	virtual void Dispose();
	bool IsDisposing();
	SimpleEvent DisposeEvent;

	static unsigned int __stdcall ListenerThread(void* data);
	SimpleSafeList<BarbaSocketServer*> ListenerSockets;
	SimpleSafeList<HANDLE> ListenerThreads;


};

