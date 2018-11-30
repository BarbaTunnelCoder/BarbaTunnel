#pragma once
#include "BarbaException.h"
#include "SimpleCriticalSection.h"

class BarbaSocketException : public BarbaException
{
public:
	virtual ~BarbaSocketException(){}
	explicit BarbaSocketException (int socketError)
	{
		SocketError = socketError;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, socketError, 0,
			Description, _countof(Description), NULL);
	}
	int GetSocketError() { return SocketError; }

private:
	int SocketError;
};

class BarbaSocket
{
public:
	explicit BarbaSocket(int af, int type, int protocol);
	explicit BarbaSocket(SOCKET s, u_long remoteIp);
	virtual ~BarbaSocket();
	//@return 0 if connection closed
	size_t Receive(BYTE* buf, size_t bufCount, bool waitAll);
	size_t Send(BYTE* buf, size_t bufCount);
	void Close();
	bool IsWritable();
	bool IsOpen() {return this->_Socket!=NULL;}
	//@return empty string if not found
	std::string ReadHttpRequest(int maxlen=5000);
	size_t GetSentBytesCount() {return this->SentBytesCount;}
	size_t GetReceiveBytesCount() {return this->ReceivedBytesCount;}
	void SetSendBufferSize(int value);
	int GetSendBufferSize();
	void SetNoDelay(bool value);
	void SetKeepAlive(bool value);
	void SetKeepAliveVal(bool enabled, u_long  keepalivetime, u_long  keepaliveinterval);
	void SetReceiveTimeOut(DWORD milisecond);
	DWORD GetReceiveTimeOut();
	void SetSendTimeOut(DWORD milisecond);
	DWORD GetSendTimeOut();
	u_long GetLastReceivedTime() {return this->LastReceivedTime;}
	u_long GetLastSentTime() {return this->LastSentTime;}
	u_long GetRemoteIp() { return this->RemoteIp;}
	bool IsReceiving() {return this->_IsReceiving;}
	static bool InitializeLib(); 
	static void UninitializeLib(); 
	void SendTo(DWORD ip, BYTE* buffer, size_t bufferLen);
	SOCKET GetSocket() {return this->_Socket;}
	u_short GetLocalPort();

protected:
	volatile u_long LastReceivedTime;
	volatile u_long LastSentTime;
	volatile u_long SentBytesCount;
	volatile u_long ReceivedBytesCount;
	u_long RemoteIp;
	BarbaSocket();
	SOCKET _Socket;
	static void ThrowSocketError();
	static void ThrowSocketError(int error);
	void Init();

private:
	volatile bool _IsReceiving;
	static int recv(SOCKET s, char* buf, int len, int flags);
};  

//BarbaSocketClient
class BarbaSocketClient : public BarbaSocket 
{
public:
	explicit BarbaSocketClient(u_long serverIp, u_short port);
	virtual ~BarbaSocketClient(){}
	u_long GetRemotePort() { return RemotePort;}

private:
	u_long RemotePort;
};


//BarbaSocketServer 
class BarbaSocketServer : public BarbaSocket 
{
public:
	explicit BarbaSocketServer(u_short port, DWORD ipAddress=0);
	virtual ~BarbaSocketServer(){}
	BarbaSocket* Accept();
};
