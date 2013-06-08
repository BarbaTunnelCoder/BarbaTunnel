#pragma once
#include "BarbaCourierTcpClient.h"

//BarbaCourierHttpClient
class BarbaCourierHttpClient : public BarbaCourierTcpClient
{
public:
	struct CreateStrcutHttp : public CreateStrcutTcp
	{
		BarbaCourierRequestMode RequestMode;
		std::tstring HostName;
	};

public:
	BarbaCourierHttpClient(CreateStrcutHttp* cs);

protected:
	virtual ~BarbaCourierHttpClient();
	void ClientWorker(ClientWorkerData* clientWorkerData) override;
	void BeforeSendMessage(BarbaSocket* barbaSocket, BarbaBuffer* messageBuffer) override;
	void AfterSendMessage(BarbaSocket* barbaSocket, bool isTransferFinished) override;
	void BeforeReceiveMessage(BarbaSocket* barbaSocket, size_t* chunkSize) override;
	void AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength, bool isTransferFinished) override;
	virtual std::tstring GetHttpPostTemplate(bool bombardMode)=0;
	virtual std::tstring GetHttpGetTemplate(bool bombardMode)=0;
	virtual void GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew)=0;
	CreateStrcutHttp* GetCreateStruct() {return (CreateStrcutHttp*)BarbaCourierStream::GetCreateStruct();}
	std::tstring CreateRequestString(bool outgoing, size_t transferSize, size_t fileHeaderSize);
	
private:
	void WaitForIncomingFileHeader(BarbaSocket* socket, size_t fileHeaderSize);
	void WaitForAcceptPostRequest(BarbaSocket* socket);
	size_t SendGetRequest(BarbaSocket* socket);
	void SendGetRequestBombard(BarbaSocket* socket, BarbaBuffer* content);
	//@return count of byte that can be sent with this request
	size_t SendPostRequest(BarbaSocket* socket, bool initBombard);
	void GetPostRequestBombard(size_t dataLength, BarbaBuffer* requestBuffer);
	BarbaCourierRequestMode* GetRequestMode() { return &GetCreateStruct()->RequestMode; }
};
