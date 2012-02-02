#pragma once
#include "barbacourier.h"

//BarbaCourierClient
class BarbaCourierClient : public BarbaCourier
{
private:
	//used to pass data to created thread
	struct ClientThreadData
	{
		ClientThreadData(BarbaCourier* courier, bool isOutgoing) {this->Courier=courier; this->IsOutgoing = isOutgoing;}
		BarbaCourier* Courier;
		bool IsOutgoing;
	};

public:
	BarbaCourierClient(BarbaCourierCreateStrcut* cs, DWORD remoteIp, u_short remotePort);
	bool IsServer() {return false;}

protected:
	virtual ~BarbaCourierClient();
	
private:
	DWORD RemoteIp;
	u_short RemotePort;
	static unsigned int __stdcall ClientWorkerThread(void* clientThreadData);
	static unsigned int __stdcall CheckKeepAliveThread(void* BarbaCourier);
	//@fakeFileHeader if null send incoming request
	size_t SendFakeRequest(BarbaSocket* socket, BarbaBuffer* fakeFileHeader);
	void CheckKeepAlive();
};
