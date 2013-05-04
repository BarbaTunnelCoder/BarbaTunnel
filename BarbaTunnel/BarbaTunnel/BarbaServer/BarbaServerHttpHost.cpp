#include "StdAfx.h"
#include "BarbaServerHttpHost.h"
#include "BarbaServerHttpConnection.h"
#include "BarbaServerApp.h"
#include "BarbaCrypt.h"
#include "BarbaUtils.h"


BarbaServerHttpHost::BarbaServerHttpHost(void)
	: DisposeEvent(true, false)
{
}

void BarbaServerHttpHost::Log(LPCTSTR format, ...)
{
	va_list argp;
	va_start(argp, format);
	TCHAR msg[1000];
	_vstprintf_s(msg, format, argp);
	va_end(argp);

	TCHAR msg2[1000];
	_stprintf_s(msg2, _T("HttpHost: TID: %4x, %s"), GetCurrentThreadId(), msg);
	BarbaLog2(msg2);
}


BarbaServerHttpHost::~BarbaServerHttpHost(void)
{
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

std::tstring BarbaServerHttpHost::GetRequestDataFromHttpRequest(LPCTSTR httpRequest, LPCTSTR keyName, BarbaBuffer* key)
{
	try
	{
		std::tstring requestDataEnc = BarbaUtils::GetKeyValueFromString(httpRequest, keyName);
		if (requestDataEnc.empty())
			return _T("");
		
		std::vector<BYTE> decodeBuffer;
		Base64::decode(requestDataEnc, decodeBuffer);
		BarbaBuffer requestDataBuf(&decodeBuffer);
		BarbaCrypt::Crypt(&requestDataBuf, key, 0, false);
		requestDataBuf.append((BYTE)0);
		requestDataBuf.append((BYTE)0);
		std::string ret = (char*)requestDataBuf.data(); //should be ANSI
		return ret;
	}
	catch (...)
	{
	}

	return _T("");
}

unsigned int BarbaServerHttpHost::AnswerThread(void* data)
{
	bool success = false;
	AnswerThreadData* threadData = (AnswerThreadData*)data;
	BarbaServerHttpHost* _this = (BarbaServerHttpHost*)threadData->HttpHost;
	BarbaSocket* socket = (BarbaSocket*)threadData->Socket;
	std::tstring clientIp = BarbaUtils::ConvertIpToString(socket->GetRemoteIp());
	socket->SetReceiveTimeOut(30000);

	try
	{
		//read httpRequest
		_this->Log(_T("New incoming connection. ServerPort: %d, ClientIp: %s."), threadData->ServerPort, clientIp.data());
		_this->Log(_T("Waiting for HTTP request."));
		std::string httpRequest = socket->ReadHttpRequest();
		bool isGet = httpRequest.size()>=3 && _strnicmp(httpRequest.data(), "GET", 3)==0;
		bool isPost = httpRequest.size()>=4 && _strnicmp(httpRequest.data(), "POST", 4)==0;
		if (!isGet && !isPost)
			throw new BarbaException(_T("Could not find GET or POST from HTTP request!"));
		bool isOutgoing = isGet;

		//find session
		std::tstring requestData = GetRequestDataFromHttpRequest(httpRequest.data(), threadData->Config->RequestDataKeyName.data(), &threadData->Config->Key);
		std::tstring sessionIdStr = BarbaUtils::GetKeyValueFromString(requestData.data(), _T("session"));
		u_long sessionId = strtoul(sessionIdStr.data(), NULL, 0);
		if (sessionId==0)
			throw new BarbaException( _T("Could not extract sessionId from HTTP request!") );

		//find connection by session id
		SimpleLock lock(&_this->CreateConnectionCriticalSection);
		BarbaServerHttpConnection* conn = (BarbaServerHttpConnection*)theServerApp->ConnectionManager.FindBySessionId(sessionId);

		//create new connection if session not found
		if (conn==NULL)
		{
			conn = theServerApp->ConnectionManager.CreateHttpConnection(threadData->Config, socket->GetRemoteIp(), threadData->ServerPort, sessionId);
			conn->Init(requestData.data());
		}
		lock.Unlock();

		//add socket to HTTP connection
		if (conn!=NULL)
			success = conn->AddSocket(socket, httpRequest.data(), isOutgoing);

		//report connection added
		if (success)
			_this->Log(_T("HTTP %s connection added to courier. SessionId: %x."), isOutgoing ? _T("GET") : _T("POST"), sessionId);
	}
	catch(BarbaException* er)
	{
		_this->Log(_T("Error: %s"), er->ToString());
		delete er;
	}
	catch(...)
	{
		_this->Log(_T("Unknown Error!"));
	}

	_this->AnswerSockets.Remove(socket);
	if (!success) delete socket;
	delete threadData;
	return success ? 0 : 1;
}


unsigned int BarbaServerHttpHost::ListenerThread(void* data)
{
	ListenerThreadData* threadData = (ListenerThreadData*)data;
	BarbaServerHttpHost* _this = (BarbaServerHttpHost*)threadData->HttpHost;
	BarbaSocketServer* listenerSocket = (BarbaSocketServer*)threadData->SocketServer;

	try
	{
		while (!_this->IsDisposing())
		{
			BarbaSocket* socket = listenerSocket->Accept(); 
			_this->AnswerSockets.AddTail(socket);
			AnswerThreadData* answerThreadData = new AnswerThreadData(_this, socket, threadData->Config, listenerSocket->GetListenPort());
			_this->AnswerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, AnswerThread, answerThreadData, 0, NULL));
			BarbaServerApp::CloseFinishedThreadHandle(&_this->AnswerThreads);
		}
	}
	catch(BarbaException* er)
	{
		_this->Log(_T("Error: %s"), er->ToString());
		delete er;
	}
	catch(...)
	{
		_this->Log(_T("Unknown Error!"));
	}

	_this->ListenerSockets.Remove(listenerSocket);
	_this->Log(_T("TCP listener closed, port: %d."), listenerSocket->GetListenPort());
	delete listenerSocket;
	delete threadData;
	return 0;
}

void BarbaServerHttpHost::AddListenerPort(BarbaServerConfig* config, u_short port)
{
	BarbaSocketServer* listenSocket = new BarbaSocketServer(port, config->ServerIp);
	ListenerSockets.AddTail(listenSocket);
	ListenerThreadData* threadData = new ListenerThreadData(this, listenSocket, config);
	ListenerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, ListenerThread, threadData, 0, NULL));
}

void BarbaServerHttpHost::Start()
{
	//Initialize listeners
	int createdSocket = 0;
	for (size_t i=0; i<theServerApp->Configs.size(); i++)
	{
		BarbaServerConfig* item = &theServerApp->Configs[i];
		if (item->Mode!=BarbaModeHttpTunnel)
			continue;

		//Initialize HttpHost to listen to ports
		for (size_t j=0; j<item->TunnelPorts.size(); j++)
		{
			PortRange* portRange = &item->TunnelPorts[j];
			for (u_short port=portRange->StartPort; port<=portRange->EndPort; port++)
			{
				//check added port count
				if (createdSocket>=BARBA_ServerMaxListenSockets)
				{
					Log(_T("Error: HTTP Tunnel could not listen more than %d ports!"), BARBA_ServerMaxListenSockets);
					j = item->TunnelPorts.size();
					break;
				}

				//add port
				try
				{
					Log(_T("Listening to TCP %s:%d."), item->ServerAddress.data(), portRange->StartPort);
					this->AddListenerPort(item, port);
					createdSocket++;
				}
				catch (BarbaException* er)
				{
					Log(_T("Error! Could not listen to TCP %s:%d. %s"), item->ServerAddress.data(), portRange->StartPort, er->ToString());
					delete er;
				}
			}
		}// for j
	}
}