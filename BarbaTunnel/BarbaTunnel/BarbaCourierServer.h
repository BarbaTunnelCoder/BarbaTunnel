#pragma once
#include "barbacourier.h"

//BarbaCourierServer
class BarbaCourierServer : public BarbaCourier
{
private:
	//used to pass data to created thread
	struct ServerThreadData
	{
		ServerThreadData(BarbaCourier* courier, BarbaSocket* socket, LPCSTR httpRequest, bool IsOutgoing) { this->Courier=courier; this->Socket=socket; this->HttpRequest = httpRequest, this->IsOutgoing = IsOutgoing; }
		BarbaCourier* Courier;
		BarbaSocket* Socket;
		std::string HttpRequest;
		bool IsOutgoing;
	};

public:
	explicit BarbaCourierServer(BarbaCourierCreateStrcut* cs);
	//@return false if no new connection accepted and caller should delete the socket
	//@return true if connection accepted, in this case caller should not delete the socket and it will be deleted automatically
	bool AddSocket(BarbaSocket* Socket, LPCSTR httpRequest, bool isOutgoing);

protected:
	virtual ~BarbaCourierServer(void);

private:
	//@return the entire fake file size
	u_int SendFakeReply(BarbaSocket* socket, LPCTSTR httpRequest, SimpleBuffer* fakeFileHeader);
	static unsigned int __stdcall ServerWorkerThread(void* serverThreadData);
};
