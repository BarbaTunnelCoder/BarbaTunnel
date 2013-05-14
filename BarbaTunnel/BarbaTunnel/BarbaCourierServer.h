#pragma once
#include "barbacourier.h"

//BarbaCourierServer
class BarbaCourierServer : public BarbaCourier
{
private:
	//used to pass data to created thread
	struct ServerThreadData
	{
		ServerThreadData(BarbaCourier* courier, BarbaSocket* socket, LPCSTR httpRequest, LPCTSTR requestData, bool IsOutgoing) 
		{ 
			this->Courier = courier; 
			this->Socket = socket; 
			this->HttpRequest = httpRequest;
			this->RequestData = requestData; 
			this->IsOutgoing = IsOutgoing; 
		}
		BarbaCourier* Courier;
		BarbaSocket* Socket;
		std::string HttpRequest;
		std::string RequestData;
		bool IsOutgoing;
	};

public:
	explicit BarbaCourierServer(BarbaCourier::CreateStrcutBag* cs);
	void Init(LPCTSTR requestData);

	//@return false if no new connection accepted and caller should delete the socket
	//@return true if connection accepted, in this case caller should not delete the socket and it will be deleted automatically
	bool AddSocket(BarbaSocket* Socket, LPCSTR httpRequest, LPCTSTR requestData, bool isOutgoing);
	bool IsServer() {return true;}

protected:
	void BeforeSendMessage(BarbaSocket* barbaSocket, BarbaBuffer* messageBuffer) override;
	void AfterSendMessage(BarbaSocket* barbaSocket, bool isTransferFinished) override;
	void BeforeReceiveMessage(BarbaSocket* barbaSocket, size_t* chunkSize) override;
	void AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength, bool isTransferFinished) override;
	virtual std::tstring GetHttpPostReplyRequest(bool bombardMode)=0;
	virtual std::tstring GetHttpGetReplyRequest(bool bombardMode)=0;
	virtual ~BarbaCourierServer(void);

private:
	void SendPostReply(BarbaSocket* socket);
	void SendPostReply(BarbaSocket* socket, BarbaBuffer* content);
	//@return the entire fake file size
	size_t SendGetReply(BarbaSocket* socket, LPCTSTR fileUrl, size_t fileSize, BarbaBuffer* fakeFileHeader);
	void GetGetReplyBombard(size_t dataLength, BarbaBuffer* requestBuffer);
	static unsigned int __stdcall ServerWorkerThread(void* serverThreadData);
};
