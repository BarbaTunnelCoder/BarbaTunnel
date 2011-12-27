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

protected:
	virtual ~BarbaCourierClient();
	
private:
	DWORD RemoteIp;
	u_short RemotePort;
	static unsigned int __stdcall ClientWorkerThread(void* clientThreadData);
	//@fakeFileHeader if null send incoming request
	u_int SendFakeRequest(BarbaSocket* socket, std::vector<BYTE>* fakeFileHeader);
};
