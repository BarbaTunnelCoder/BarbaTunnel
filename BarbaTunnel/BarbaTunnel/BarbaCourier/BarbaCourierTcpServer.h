#pragma once
#include "BarbaCourier.h"

//BarbaCourierTcpServer
class BarbaCourierTcpServer : public BarbaCourier
{
public:
	struct CreateStrcutTcp : public CreateStrcut
	{
	};

protected:
	struct ServerWorkerData
	{
		BarbaCourierTcpServer* Courier; //for internal use
		BarbaSocket* Socket;
		std::tstring RequestData;
		std::tstring RequestString;
	};
	virtual void ServerWorker(ServerWorkerData* serverWorkerData);
	virtual ~BarbaCourierTcpServer(void);

private:
	static unsigned int __stdcall ServerWorkerThread(void* serverWorkerThreadData);
	
public:
	explicit BarbaCourierTcpServer(CreateStrcutTcp* cs);
	virtual void Init(LPCTSTR requestData);

	//@return false if no new connection accepted and caller should delete the socket
	//@return true if connection accepted, in this case caller should not delete the socket and it will be deleted automatically
	void AddSocket(BarbaSocket* Socket, LPCTSTR requestString, LPCTSTR requestData);
	bool IsServer() {return true;}
};
