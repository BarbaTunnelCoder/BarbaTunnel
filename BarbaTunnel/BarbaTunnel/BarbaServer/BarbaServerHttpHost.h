/*
 * BarbaServerHttpHost class
 * this class just used for HTTP-Tunneling
 */

#pragma once
#include "General.h"
#include "BarbaServerConfig.h"
#include "BarbaServerHttpCourier.h"

class BarbaServerHttpHost
{
private:
	//used to pass data to created thread
	struct ListenerThreadData
	{
		ListenerThreadData(BarbaServerHttpHost* httpServer, BarbaSocketServer* socketServer, BarbaServerConfigItem* configItem) { this->HttpServer=httpServer; this->SocketServer=socketServer; this->ConfigItem = configItem;}
		BarbaServerHttpHost* HttpServer;
		BarbaSocketServer* SocketServer;
		BarbaServerConfigItem* ConfigItem;
	};

	//used to pass data to created thread
	struct AnswerThreadData
	{
		AnswerThreadData(BarbaServerHttpHost* httpServer, BarbaSocket* socket, BarbaServerConfigItem* configItem, u_short serverPort) { this->HttpServer=httpServer; this->Socket=socket; this->ConfigItem = configItem; this->ServerPort = serverPort;}
		BarbaServerHttpHost* HttpServer;
		u_short ServerPort;
		BarbaSocket* Socket;
		BarbaServerConfigItem* ConfigItem;
	};

public:
	explicit BarbaServerHttpHost();
	virtual ~BarbaServerHttpHost(void);
	void Initialize();

protected:
	void AddListenerPort(BarbaServerConfigItem* configItem, u_short port);

private:
	SimpleCriticalSection CreateConnectionCriticalSection;
	virtual void Dispose();
	bool IsDisposing();
	SimpleEvent DisposeEvent;

	static u_long ExtractSessionId(LPCSTR header);
	static unsigned int __stdcall AnswerThread(void* answerThreadData);
	static unsigned int __stdcall ListenerThread(void* listenerThreadData);
	SimpleSafeList<BarbaSocket*> ListenerSockets;
	SimpleSafeList<BarbaSocket*> AnswerSockets;
	SimpleSafeList<HANDLE> ListenerThreads;
	SimpleSafeList<HANDLE> AnswerThreads;


};

