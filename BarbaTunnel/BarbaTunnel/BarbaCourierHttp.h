#pragma once
#include "BarbaCourier.h"

//BarbaCourier
class BarbaCourierHttp : public BarbaCourier
{
public:
	//@maxConnenction number of simultaneous connection for each outgoing and incoming, eg: 2 mean 2 connection for send and 2 connection for receive so the total will be 4
	explicit BarbaCourierHttp(CreateStrcutBag* cs);
	std::tstring GetRequestDataFromHttpRequest(LPCTSTR httpRequest);
	static std::tstring BarbaCourierHttp::GetRequestDataFromHttpRequest(LPCTSTR httpRequest, LPCTSTR keyName, BarbaBuffer* key);
	void RefreshParameters() override;

protected:
	virtual ~BarbaCourierHttp();
	virtual void GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew);
	void SendFileHeader(BarbaSocket* socket, BarbaBuffer* fakeFileHeader);
	void WaitForIncomingFileHeader(BarbaSocket* socket, size_t fileHeaderSize);
	void InitRequestVars(std::tstring& src, LPCTSTR filename, LPCTSTR contentType, size_t transferSize, size_t fileHeaderSize, bool outgoing);
	bool IsBombardGet; 
	bool IsBombardGetPayload; 
	bool IsBombardPost;  
	bool IsBombardPostReply;  
	bool IsBombardPostReplyPayload;  
};

