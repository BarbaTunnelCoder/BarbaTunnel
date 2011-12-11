#include "StdAfx.h"
#include "BarbaSocket.h"

BarbaSocket::BarbaSocket()
{
	InitializeLib();
	_Socket = ::socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
	if (_Socket == INVALID_SOCKET)
		ThrowSocketError();
}

BarbaSocket::BarbaSocket(SOCKET s)
{
	InitializeLib();
	_Socket = s;
}

void BarbaSocket::ThrowSocketError(int er)
{
	TCHAR err[1000];
	_tcserror_s(err, er);
	throw err;
}

void BarbaSocket::ThrowSocketError()
{
	ThrowSocketError(::WSAGetLastError());
}

bool BarbaSocket::IsWritable()
{
	return send(this->_Socket, NULL, 0, 0)!=SOCKET_ERROR;
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


int BarbaSocket::Receive(BYTE* buf, size_t bufCount, bool waitAll)
{
	int flags = 0;
	if (waitAll) flags |= MSG_WAITALL;
	int ret = ::recv(_Socket, (char*)buf, bufCount, flags);
	if (ret==SOCKET_ERROR)
		ThrowSocketError();
	return ret;
}

int BarbaSocket::Send(BYTE* buf, size_t bufCount)
{
	int ret = send(_Socket, (char*)buf, bufCount, 0);
	if (ret==SOCKET_ERROR)
		ThrowSocketError();
	return ret;
}

std::string BarbaSocket::ReadHttpHeader(int maxlen)
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


BarbaSocketClient::BarbaSocketClient(DWORD serverIp, u_short port)
{
	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = serverIp;
	if (::connect(_Socket, (sockaddr*)&addr, sizeof sockaddr)==SOCKET_ERROR)
	{
		TCHAR err[1000];
		_tcserror_s(err, WSAGetLastError());
		throw err;
	}
}

BarbaSocketServer::BarbaSocketServer(u_short port) 
{
	sockaddr_in sa = {0};

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
	SOCKET newSocket = accept(_Socket, 0, 0);
	if (newSocket == INVALID_SOCKET) 
		ThrowSocketError();

  BarbaSocket* ret = new BarbaSocket(newSocket);
  return ret;
}
