#include "StdAfx.h"
#include "BarbaServerTcpHost.h"
#include "BarbaServerTcpConnection.h"
#include "BarbaServerApp.h"
#include "BarbaCrypt.h"
#include "BarbaUtils.h"


BarbaServerTcpHost::BarbaServerTcpHost(void)
	: DisposeEvent(true, false)
{
}

void BarbaServerTcpHost::Log(LPCTSTR format, ...)
{
	va_list argp;
	va_start(argp, format);
	TCHAR msg[1000];
	_vstprintf_s(msg, format, argp);
	va_end(argp);

	TCHAR msg2[1000];
	_stprintf_s(msg2, _T("HttpHost: TID: %4x, %s"), GetCurrentThreadId(), msg);
	BarbaLog1(msg2);
}

void BarbaServerTcpHost::Log2(LPCTSTR format, ...)
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


BarbaServerTcpHost::~BarbaServerTcpHost(void)
{
}

void BarbaServerTcpHost::Dispose()
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

std::tstring BarbaServerTcpHost::CreateRequestDataKeyName(BarbaBuffer* key)
{
	std::string keyName = "RequestDataKey";
	BarbaBuffer keyBuffer((BYTE*)keyName.data(), keyName.size());
	BarbaCrypt::Crypt(&keyBuffer, key, 0, true);
	std::tstring ret = Base64::encode(keyBuffer.data(), keyBuffer.size());
	StringUtils::ReplaceAll(ret, "=", "");
	StringUtils::ReplaceAll(ret, "/", "");
	return ret;

}
std::tstring BarbaServerTcpHost::GetRequestDataFromHttpRequest(LPCTSTR httpRequest, LPCTSTR keyName, BarbaBuffer* key)
{
	try
	{
		std::tstring requestDataEnc = BarbaUtils::GetKeyValueFromString(httpRequest, keyName);
		if (requestDataEnc.empty())
			return _T("");

		std::vector<BYTE> decodeBuffer;
		Base64::decode(requestDataEnc, decodeBuffer);
		BarbaBuffer requestDataBuf(decodeBuffer.data(), decodeBuffer.size());
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

unsigned int BarbaServerTcpHost::AnswerThread(void* data)
{
	AnswerWorkerData* workerData = (AnswerWorkerData*)data;
	workerData->HttpHost->AnswerWorker(workerData);
	delete workerData;
	return 0;
}

unsigned int BarbaServerTcpHost::ListenerThread(void* data)
{
	ListenerWorkerData* workerData = (ListenerWorkerData*)data;
	workerData->HttpHost->ListenerWorker(workerData);
	delete workerData;
	return 0;
}

void BarbaServerTcpHost::AnswerWorker(AnswerWorkerData* workerData)
{
	bool success = false;
	BarbaSocket* socket = workerData->Socket;
	std::tstring clientIp = BarbaUtils::ConvertIpToString(socket->GetRemoteIp(), theApp->LogAnonymously);
	socket->SetReceiveTimeOut(30000);

	try
	{
		//read httpRequest
		Log2(_T("New incoming connection. ServerPort: %d, ClientIp: %s."), workerData->ServerPort, clientIp.data());
		Log2(_T("Waiting for request."));
		std::string requestString = socket->ReadHttpRequest();

		//find session
		std::tstring requestDataKeyName = CreateRequestDataKeyName(&workerData->Config->Key);
		std::tstring requestData = GetRequestDataFromHttpRequest(requestString.data(), requestDataKeyName.data(), &workerData->Config->Key);
		Log2(_T("Request recieved. RequestData: %s"), requestData.data());
		u_long sessionId = BarbaUtils::GetKeyValueFromString(requestData.data(), _T("SessionId"), 0);
		if (sessionId==0)
			throw new BarbaException( _T("Could not extract sessionId from request!") );

		//find connection by session id and create new if not found
		SimpleLock lock(&CreateConnectionCriticalSection);
		BarbaServerTcpConnectionBase* conn = (BarbaServerTcpConnectionBase*)theServerApp->ConnectionManager.FindBySessionId(sessionId);
		if (conn==NULL)
			conn = theServerApp->ConnectionManager.CreateTcpConnection(workerData->Config, socket->GetRemoteIp(), requestData.data());
		lock.Unlock();

		//add socket to HTTP connection
		if (conn!=NULL)
			conn->AddSocket(socket, requestString.data(), requestData.data());

		success = true;
	}
	catch(BarbaException* er)
	{
		Log2(_T("Error: %s"), er->ToString());
		delete er;
	}
	catch(...)
	{
		Log2(_T("Unknown Error!"));
	}

	AnswerSockets.Remove(socket);
	if (!success) delete socket;
}

void BarbaServerTcpHost::ListenerWorker(ListenerWorkerData* workerData)
{
	BarbaSocketServer* listenerSocket = workerData->SocketServer;
	u_short localPort = listenerSocket->GetLocalPort();

	try
	{
		while (!IsDisposing())
		{
			BarbaSocket* socket = listenerSocket->Accept(); 
			AnswerSockets.AddTail(socket);
			AnswerWorkerData* answerWorkerData = new AnswerWorkerData(this, socket, workerData->Config, listenerSocket->GetLocalPort());
			AnswerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, AnswerThread, answerWorkerData, 0, NULL));
			BarbaServerApp::CloseFinishedThreadHandle(&AnswerThreads);
		}
	}
	catch(BarbaSocketException* er)
	{
		bool isDisposeCancelError = IsDisposing() &&  er->GetSocketError()==WSAEINTR;
		if ( !isDisposeCancelError )
			Log(_T("Error: %s"), er->ToString(), er->GetSocketError());
		delete er;
	}
	catch(BarbaException* er)
	{
		Log(_T("Error: %s"), er->ToString());
		delete er;
	}
	catch(...)
	{
		Log(_T("Unknown Error!"));
	}

	ListenerSockets.Remove(listenerSocket);
	Log2(_T("TCP listener closed, port: %d."), localPort);
	delete listenerSocket;
}


void BarbaServerTcpHost::AddListenerPort(BarbaServerConfig* config, u_short port)
{
	BarbaSocketServer* listenSocket = new BarbaSocketServer(port, config->ServerIp);
	ListenerSockets.AddTail(listenSocket);
	ListenerWorkerData* threadData = new ListenerWorkerData(this, listenSocket, config);
	ListenerThreads.AddTail( (HANDLE)_beginthreadex(NULL, BARBA_SocketThreadStackSize, ListenerThread, threadData, 0, NULL));
}

void BarbaServerTcpHost::Start()
{
	//Initialize listeners
	int createdSocket = 0;
	for (size_t i=0; i<theServerApp->Configs.size() && createdSocket<BARBA_ServerMaxListenSockets; i++)
	{
		BarbaServerConfig* item = &theServerApp->Configs[i];
		if (item->Mode!=BarbaModeTcpTunnel && item->Mode!=BarbaModeHttpTunnel)
			continue;

		BarbaArray<u_short> ports;
		item->TunnelPorts.GetAllPorts(&ports);

		//Initialize HttpHost to listen to ports
		for (size_t j=0; j<ports.size(); j++)
		{
			u_short port = ports[j];
			//check added port count
			if (createdSocket>=BARBA_ServerMaxListenSockets)
			{
				Log(_T("Error: Could not listen more than %d TCP ports!"), BARBA_ServerMaxListenSockets);
				break;
			}

			//serverAddress
			std::tstring serverAddress = item->ServerAddress;
			if (theApp->LogAnonymously)
				serverAddress = BarbaUtils::ConvertIpToString(item->ServerIp, true);

			//add port
			try
			{
				Log(_T("Listening to TCP %s:%d."), serverAddress.data(), port);
				AddListenerPort(item, port);
				createdSocket++;
			}
			catch (BarbaException* er)
			{
				Log(_T("Error! Could not listen to TCP %s:%d. %s"), serverAddress.data(), port, er->ToString());
				delete er;
			}
		}// for j
	}
}