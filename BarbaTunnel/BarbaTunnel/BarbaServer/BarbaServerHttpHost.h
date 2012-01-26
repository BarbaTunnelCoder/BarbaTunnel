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
		ListenerThreadData(BarbaServerHttpHost* httpHost, BarbaSocketServer* socketServer, BarbaServerConfig* config) { this->HttpHost=httpHost; this->SocketServer=socketServer; this->Config = config;}
		BarbaServerHttpHost* HttpHost;
		BarbaSocketServer* SocketServer;
		BarbaServerConfig* Config;
	};

	//used to pass data to created thread
	struct AnswerThreadData
	{
		AnswerThreadData(BarbaServerHttpHost* httpHost, BarbaSocket* socket, BarbaServerConfig* config, u_short serverPort) { this->HttpHost=httpHost; this->Socket=socket; this->Config = config; this->ServerPort = serverPort;}
		BarbaServerHttpHost* HttpHost;
		u_short ServerPort;
		BarbaSocket* Socket;
		BarbaServerConfig* Config;
	};

public:
	explicit BarbaServerHttpHost();
	virtual ~BarbaServerHttpHost(void);
	virtual void Dispose();
	void Start();

private:
	static std::tstring GetRequestDataFromHttpRequest(LPCTSTR httpRequest, LPCTSTR keyName, std::vector<BYTE>* key);
	void AddListenerPort(BarbaServerConfig* config, u_short port);
	void Log(LPCTSTR format, ...);
	SimpleCriticalSection CreateConnectionCriticalSection;
	bool IsDisposing() { return this->DisposeEvent.IsSet(); }
	SimpleEvent DisposeEvent;

	static unsigned int __stdcall AnswerThread(void* answerThreadData);
	static unsigned int __stdcall ListenerThread(void* listenerThreadData);
	SimpleSafeList<BarbaSocket*> ListenerSockets;
	SimpleSafeList<BarbaSocket*> AnswerSockets;
	SimpleSafeList<HANDLE> ListenerThreads;
	SimpleSafeList<HANDLE> AnswerThreads;

};

