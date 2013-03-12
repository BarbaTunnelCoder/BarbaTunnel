#include "StdAfx.h"
#include "BarbaSocket.h"
#define MSG_WAITALL     0x8             /* do not complete until packet is completely filled */

BarbaSocket::BarbaSocket(int af, int type, int protocol)
{
	Init();
	this->_Socket = ::socket(af, type, protocol);
	if (_Socket == INVALID_SOCKET)
		ThrowSocketError();
}

BarbaSocket::BarbaSocket()
{
	Init();
	this->_Socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_Socket == INVALID_SOCKET)
		ThrowSocketError();
}

BarbaSocket::BarbaSocket(SOCKET s, u_long remoteIp)
{
	Init();
	_Socket = s;
	this->RemoteIp = remoteIp;
}

void BarbaSocket::Init()
{
	this->RemoteIp = 0;
	this->SentBytesCount = 0;
	this->ReceivedBytesCount = 0;
	this->LastReceivedTime = 0;
	this->LastSentTime = this->LastReceivedTime = GetTickCount();
	this->_IsReceiving = false;
	this->_Socket = NULL;
	InitializeLib();
}

void BarbaSocket::ThrowSocketError()
{
	throw new BarbaSocketException(::WSAGetLastError());
}

void BarbaSocket::ThrowSocketError(int error)
{
	throw new BarbaSocketException(error);
}

bool BarbaSocket::IsWritable()
{
	return send(this->_Socket, NULL, 0, 0)!=SOCKET_ERROR;
}

void BarbaSocket::SetReceiveTimeOut(DWORD milisecond)
{
	//WINDOWS: Timeout value is a DWORD in milliseconds, address passed to setsockopt() is const char *
	//LINUX: Timeout value is a struct timeval, address passed to setsockopt() is const void *
	int res = setsockopt(this->_Socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&milisecond, sizeof(milisecond));
	if (res==SOCKET_ERROR)
		ThrowSocketError();
}

void BarbaSocket::SetSendTimeOut(DWORD milisecond)
{
	//WINDOWS: Timeout value is a DWORD in milliseconds, address passed to setsockopt() is const char *
	//LINUX: Timeout value is a struct timeval, address passed to setsockopt() is const void *
	int res = setsockopt(this->_Socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&milisecond, sizeof(milisecond));
	if (res==SOCKET_ERROR)
		ThrowSocketError();
}

void BarbaSocket::SetNoDelay(bool value)
{
	int flag = value ? 1 : 0;
	int res = setsockopt(this->_Socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag,  sizeof(int));
	if (res==SOCKET_ERROR)
		ThrowSocketError();
}

void BarbaSocket::SetKeepAlive(bool value)
{
	int flag = value ? 1 : 0;
	int res = setsockopt(this->_Socket, SOL_SOCKET, SO_KEEPALIVE, (char *) &flag,  sizeof(int));
   	if (res==SOCKET_ERROR)
		ThrowSocketError();
}

void BarbaSocket::Close()
{
	if (_Socket!=NULL)
	{
		::shutdown(_Socket, SD_BOTH);
		::closesocket(_Socket);
		_Socket = NULL;
	}
}


BarbaSocket::~BarbaSocket()
{
	if (_Socket!=NULL)
		Close();
	UninitializeLib();
}

bool BarbaSocket::InitializeLib()
{
	WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    return ::WSAStartup(wVersionRequested, &wsaData)==0;
}

void BarbaSocket::UninitializeLib()
{
	 ::WSACleanup();
}

int BarbaSocket::recv(SOCKET s, char* buf, int len, int flags)
{
	//use original api if flag does not have MSG_WAITALL
	if ( (flags&MSG_WAITALL)==0 )
		return ::recv(s, buf, len, flags);

	//check is MSG_WAITALL supported
	static bool isWaitAllSupported = true;
	if (isWaitAllSupported)
	{
		int ret = ::recv(s, buf, len, flags);
		if (ret==SOCKET_ERROR && WSAGetLastError()==WSAEOPNOTSUPP)
			isWaitAllSupported = false;
		else
			return ret;
	}

	//custom wait all of legacy windows
	flags &= ~MSG_WAITALL;
	int ret = ::recv(s, buf, len, flags);
	if (ret==SOCKET_ERROR || ret==0)
		return ret;

	int readed = ret;
	while (readed<len)
	{
		ret = ::recv(s, (buf + readed), len-readed, flags);
		if (ret==SOCKET_ERROR || ret==0) 
			return ret;
		readed += ret;
	}
	return readed;
}

size_t BarbaSocket::Receive(BYTE* buf, size_t bufCount, bool waitAll)
{
	int flags = 0;
	if (waitAll) flags |= MSG_WAITALL;
	_IsReceiving = true;
	int ret = recv(_Socket, (char*)buf, (int)bufCount, flags);
	_IsReceiving = false;
	if (ret==SOCKET_ERROR)
		ThrowSocketError();
	this->ReceivedBytesCount += ret;
	this->LastReceivedTime = GetTickCount();
	return ret;
}

size_t BarbaSocket::Send(BYTE* buf, size_t bufCount)
{
	int ret = send(_Socket, (char*)buf, (int)bufCount, 0);
	if (ret==SOCKET_ERROR)
		ThrowSocketError();
	this->SentBytesCount += ret;
	this->LastSentTime = GetTickCount();
	return ret;
}

std::string BarbaSocket::ReadHttpRequest(int maxlen)
{
	std::string ret;
	std::string ending = "\r\n\r\n";
	for (int i=0; i<maxlen; i++)
	{
		char curChar;
		if (Receive((BYTE*)&curChar, 1, true)!=1)
			break;

		ret += curChar;
		if (ret.length()>=4)
		{
			bool findEnd = ret.compare(ret.length() - ending.length(), ending.length(), ending)==0;
			if (findEnd)
				return ret;
		}
	}

	std::string empty;
	return empty;
}

void BarbaSocket::SendTo(DWORD ip, BYTE* buffer, size_t bufferLen)
{
	sockaddr_in in = {0};
	in.sin_family = AF_INET;
	in.sin_addr.S_un.S_addr = ip;
	if (sendto(this->_Socket, (char*)buffer, (int)bufferLen, 0, (sockaddr*)&in, sizeof(in))==SOCKET_ERROR)
		ThrowSocketError(); //throw error after close socket
}


BarbaSocketClient::BarbaSocketClient(u_long serverIp, u_short port)
{
	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = serverIp;
	this->RemoteIp = serverIp;
	if (::connect(_Socket, (sockaddr*)&addr, sizeof sockaddr)==SOCKET_ERROR)
		ThrowSocketError();
}

BarbaSocketServer::BarbaSocketServer(u_short port, DWORD ipAddress) 
{
	this->ListenPort = port;
	sockaddr_in sa = {0};
	sa.sin_addr.S_un.S_addr = ipAddress;
	sa.sin_family = PF_INET;             
	sa.sin_port = htons(port);          
	_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_Socket == INVALID_SOCKET)
		ThrowSocketError();

	// bind the socket to the internet address 
	if (::bind(_Socket, (sockaddr *)&sa, sizeof(sockaddr_in)) == SOCKET_ERROR) 
		ThrowSocketError();
  
	if (listen(_Socket, SOMAXCONN)==SOCKET_ERROR)
		ThrowSocketError();
}

BarbaSocket* BarbaSocketServer::Accept() 
{
	sockaddr_in addr_in = {0};
	int addr_in_len = sizeof (addr_in);


	SOCKET newSocket = accept(_Socket, (sockaddr*)&addr_in, &addr_in_len);
	if (newSocket == INVALID_SOCKET) 
		ThrowSocketError();

	BarbaSocket* ret = new BarbaSocket(newSocket, addr_in.sin_addr.S_un.S_addr);
	return ret;
}
