#pragma once
#include "BarbaCourierTcpServer.h"

//BarbaCourierHttpServer
class BarbaCourierHttpServer : public BarbaCourierTcpServer
{
public:
	struct CreateStrcutHttp : public CreateStrcutTcp
	{
		BarbaCourierRequestMode RequestMode;
	};

public:
	explicit BarbaCourierHttpServer(CreateStrcutHttp* cs);
	void Init(LPCTSTR requestData) override;

protected:
	virtual std::tstring GetHttpPostReplyRequest(bool bombardMode)=0;
	virtual std::tstring GetHttpGetReplyRequest(bool bombardMode)=0;
	virtual void GetFakeFile(TCHAR* filename, std::tstring* contentType, BarbaBuffer* fakeFileHeader) = 0;
	virtual ~BarbaCourierHttpServer(void);

private:
	void ServerWorker(ServerWorkerData* serverWorkerData) override;
	void BeforeSendMessage(BarbaSocket* barbaSocket, BarbaBuffer* messageBuffer) override;
	void AfterSendMessage(BarbaSocket* barbaSocket, bool isTransferFinished) override;
	void BeforeReceiveMessage(BarbaSocket* barbaSocket, size_t* chunkSize) override;
	void AfterReceiveMessage(BarbaSocket* barbaSocket, size_t messageLength, bool isTransferFinished) override;
	void WaitForIncomingFileHeader(BarbaSocket* socket, size_t fileHeaderSize);
	void SendPostReply(BarbaSocket* socket);
	void SendPostReply(BarbaSocket* socket, BarbaBuffer* content);
	//@return the entire fake file size
	size_t SendGetReply(BarbaSocket* socket, LPCTSTR fileUrl, size_t fileSize, BarbaBuffer* fakeFileHeader);
	void GetGetReplyBombard(size_t dataLength, BarbaBuffer* requestBuffer);
	BarbaCourierRequestMode* GetRequestMode() { return &GetCreateStruct()->RequestMode; }
	CreateStrcutHttp* GetCreateStruct() { return (CreateStrcutHttp*)BarbaCourierTcpServer::GetCreateStruct(); }
};
