/*
 * BarbaServerHttp class
 * this class just used for HTTP-Tunneling
 */

#pragma once
#include "General.h"
#include "BarbaServerHttpCourier.h"

class BarbaServerHttp
{
private:
	//used to pass data to created thread
	struct ListenerThreadData
	{
		ListenerThreadData(BarbaServerHttp* httpServer, BarbaSocketServer* socketServer) { this->HttpServer=httpServer; this->SocketServer=socketServer; }
		BarbaServerHttp* HttpServer;
		BarbaSocketServer* SocketServer;
	};

public:
	BarbaServerHttp(void);
	virtual ~BarbaServerHttp(void);


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

