#pragma once

class BarbaSocket
{
public:
	virtual ~BarbaSocket();
	//@return 0 if connection closed
	int Receive(BYTE* buf, size_t bufCount, bool waitAll);
	int Send(BYTE* buf, size_t bufCount);
	void Close();
	bool IsWritable();
	//@return empty string if not found
	std::string ReadHttpHeader(int maxlen=5000);
	size_t GetSentBytesCount() {return this->SentBytesCount;}
	size_t GetReceiveBytesCount() {return this->ReceiveBytesCount;}
	void SetNoDelay(bool value);
	void SetReceiveTimeOut(long second);
	void SetSendTimeOut(long second);

	static bool InitializeLib(); 
	static void UninitializeLib(); 
	BarbaSocket(SOCKET s);
	BarbaSocket();

protected:
	size_t SentBytesCount;
	size_t ReceiveBytesCount;
	SOCKET _Socket;
	void ThrowSocketError(int er);
	void ThrowSocketError();
};  

//BarbaSocketClient
class BarbaSocketClient : public BarbaSocket 
{
public:
	BarbaSocketClient(DWORD serverIp, u_short port);
	virtual ~BarbaSocketClient(){}
};


//BarbaSocketServer 
class BarbaSocketServer : public BarbaSocket 
{
public:
  BarbaSocketServer(u_short port);
  virtual ~BarbaSocketServer(){}
  BarbaSocket* Accept();
};
