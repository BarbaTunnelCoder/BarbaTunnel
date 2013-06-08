#pragma once
#include "BarbaCourierStream.h"
#include "BarbaPortRange.h"

class BarbaCourierTcpClient : public BarbaCourierStream
{
public:
	struct CreateStrcutTcp : public CreateStrcut
	{
		CreateStrcutTcp() {RemoteIp=0; PortRange=NULL; MaxTransferSize=0;}
		DWORD RemoteIp;
		BarbaPortRange* PortRange;
		size_t MaxTransferSize;
	};

protected:
	struct ClientWorkerData
	{
		BarbaCourierTcpClient* Courier; //for internal use
		bool IsOutgoing;
	};

public:
	BarbaCourierTcpClient(CreateStrcutTcp* cs);
	bool IsServer() override {return false;}
	virtual void Init();
	
protected:
	std::tstring CreateRequestString(bool outgoing, size_t transferSize, LPCTSTR other=NULL);
	CreateStrcutTcp* GetCreateStruct() {return (CreateStrcutTcp*)BarbaCourierStream::GetCreateStruct();}
	virtual ~BarbaCourierTcpClient();
	virtual void ClientWorker(ClientWorkerData* clientWorkerData);
	
private:
	static unsigned int __stdcall ClientWorkerThread(void* clientWorkerThreadData);
	void StartClientThreads();
	size_t SendGetRequest(BarbaSocket* socket);
};
