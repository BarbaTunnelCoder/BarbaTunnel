/*
 * BarbaServerTcpHost class
 * this class just used for HTTP-Tunneling
 */

#pragma once
#include "General.h"
#include "BarbaServerConfig.h"
#include "BarbaSocket.h"

class BarbaServerTcpHost
{
private:
	//used to pass data to created thread
	struct ListenerWorkerData
	{
		ListenerWorkerData(BarbaServerTcpHost* httpHost, BarbaSocketServer* socketServer, BarbaServerConfig* config) { HttpHost=httpHost; SocketServer=socketServer; Config = config;}
		BarbaServerTcpHost* HttpHost;
		BarbaSocketServer* SocketServer;
		BarbaServerConfig* Config;
	};

	//used to pass data to created thread
	struct AnswerWorkerData
	{
		AnswerWorkerData(BarbaServerTcpHost* httpHost, BarbaSocket* socket, BarbaServerConfig* config, u_short serverPort) { this->HttpHost=httpHost; this->Socket=socket; this->Config = config; this->ServerPort = serverPort;}
		BarbaServerTcpHost* HttpHost;
		u_short ServerPort;
		BarbaSocket* Socket;
		BarbaServerConfig* Config;
	};

public:
	explicit BarbaServerTcpHost();
	virtual ~BarbaServerTcpHost(void);
	virtual void Dispose();
	void Start();

private:
	static std::tstring BarbaServerTcpHost::CreateRequestDataKeyName(BarbaBuffer* key);
	static std::tstring GetRequestDataFromHttpRequest(LPCTSTR httpRequest, LPCTSTR keyName, BarbaBuffer* key);
	void AddListenerPort(BarbaServerConfig* config, u_short port);
	void Log(LPCTSTR format, ...);
	void Log2(LPCTSTR format, ...);
	SimpleCriticalSection CreateConnectionCriticalSection;
	bool IsDisposing() { return this->DisposeEvent.IsSet(); }
	SimpleEvent DisposeEvent;

	void AnswerWorker(AnswerWorkerData* workerData);
	void ListenerWorker(ListenerWorkerData* workerData);
	static unsigned int __stdcall AnswerThread(void* answerThreadData);
	static unsigned int __stdcall ListenerThread(void* listenerThreadData);
	SimpleSafeList<BarbaSocket*> ListenerSockets;
	SimpleSafeList<BarbaSocket*> AnswerSockets;
	SimpleSafeList<HANDLE> ListenerThreads;
	SimpleSafeList<HANDLE> AnswerThreads;

};

