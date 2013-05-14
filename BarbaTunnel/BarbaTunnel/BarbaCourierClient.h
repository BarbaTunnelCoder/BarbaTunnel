#pragma once
#include "BarbaCourierHttp.h"

//BarbaCourierClient
class BarbaCourierClient : public BarbaCourierHttp
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
	BarbaCourierClient(BarbaCourier::CreateStrcutBag* cs, DWORD remoteIp, u_short remotePort);
	bool IsServer() {return false;}

protected:
	virtual std::tstring GetHttpPostTemplate(bool bombardMode)=0;
	virtual std::tstring GetHttpGetTemplate(bool bombardMode)=0;
	virtual ~BarbaCourierClient();
	void BeforeSendMessage(BarbaSocket* barbaSocket, BarbaBuffer* messageBuffer) override;
	void AfterSendMessage(BarbaSocket* barbaSocket, bool isTransferFinished) override;
	void BeforeReceiveMessage(BarbaSocket* barbaSocket, size_t* chunkSize) override;
	void AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength, bool isTransferFinished) override;
	
private:
	DWORD RemoteIp;
	u_short RemotePort;
	static unsigned int __stdcall ClientWorkerThread(void* clientThreadData);
	static unsigned int __stdcall CheckKeepAliveThread(void* BarbaCourier);
	void WaitForAcceptPostRequest(BarbaSocket* socket);
	size_t SendGetRequest(BarbaSocket* socket);
	void SendGetRequestBombard(BarbaSocket* socket, BarbaBuffer* content);
	//@return count of byte that can be sent with this request
	size_t SendPostRequest(BarbaSocket* socket, bool initBombard);
	void GetPostRequestBombard(size_t dataLength, BarbaBuffer* requestBuffer);
	void CheckKeepAlive();
};
