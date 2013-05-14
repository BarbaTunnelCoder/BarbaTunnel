#include "StdAfx.h"
#include "BarbaCourierHttp.h"
#include "BarbaUtils.h"


BarbaCourierHttp::BarbaCourierHttp(BarbaCourier::CreateStrcutBag* cs)
	: BarbaCourier(cs)
{
	RefreshParameters();
}

BarbaCourierHttp::~BarbaCourierHttp()
{
}

void BarbaCourierHttp::SendFileHeader(BarbaSocket* socket, BarbaBuffer* fakeFileHeader)
{
	if (fakeFileHeader->empty())
	{
		Log2(_T("Could not find fake file data. Fake file header ignored!"));
		return;
	}

	//sending fake file header
	Log2(_T("Sending fake file header. HeaderSize: %u KB."), fakeFileHeader->size()/1000);
	if (socket->Send(fakeFileHeader->data(), fakeFileHeader->size())!=(int)fakeFileHeader->size())
		throw new BarbaException(_T("Fake file header does not send!"));
}

void BarbaCourierHttp::WaitForIncomingFileHeader(BarbaSocket* socket, size_t fileHeaderSize)
{
	if (fileHeaderSize==0)
	{
		Log2(_T("Request does not have fake file header."));
	} 
	else if (fileHeaderSize>BarbaCourier_MaxFileHeaderSize)
	{
		throw new BarbaException(_T("Fake file header could not be more than %u size! Requested Size: %u."), BarbaCourier_MaxFileHeaderSize, fileHeaderSize);
	}
	else
	{
		Log2(_T("Waiting for incoming fake file header. HeaderSize: %u KB."), fileHeaderSize/1000);

		BarbaBuffer buffer(fileHeaderSize);
		if (socket->Receive(buffer.data(), buffer.size(), true)!=(int)buffer.size())
			throw new BarbaException(_T("Could not receive fake file header."));
	}
}

void BarbaCourierHttp::GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* /*fakeFileHeader*/, bool createNew)
{
	if (fileSize!=NULL)
		*fileSize = BarbaUtils::GetRandom(this->CreateStruct.FakeFileMaxSize/2, this->CreateStruct.FakeFileMaxSize); 

	if (createNew)
	{
		u_int fileNameId = BarbaUtils::GetRandom(1, UINT_MAX);
		_ltot_s(fileNameId, filename, MAX_PATH, 32);
	}
	*contentType = BarbaUtils::GetFileExtensionFromUrl(filename);

}

void BarbaCourierHttp::RefreshParameters()
{
	BarbaCourier::RefreshParameters();

	std::string bombardMode = this->CreateStruct.HttpBombardMode;
	bombardMode.append(_T(" "));
	StringUtils::MakeLower( bombardMode );
	IsBombardGet = _tcsstr(bombardMode.data(), "/get")!=NULL;
	IsBombardGetPayload = _tcsstr(bombardMode.data(), "/getpayload ")!=NULL;
	IsBombardPost = _tcsstr(bombardMode.data(), "/post ")!=NULL;
	IsBombardPostReply = _tcsstr(bombardMode.data(), "/reply ")!=NULL;
	IsBombardPostReplyPayload = _tcsstr(bombardMode.data(), "/replypayload ")!=NULL;
}

std::tstring BarbaCourierHttp::GetRequestDataFromHttpRequest(LPCTSTR httpRequest)
{
	try
	{
		std::tstring requestDataEnc = BarbaUtils::GetKeyValueFromString(httpRequest, this->CreateStruct.RequestDataKeyName.data());
		if (requestDataEnc.empty())
			return _T("");

		std::vector<BYTE> decodeBuffer;
		Base64::decode(requestDataEnc, decodeBuffer);
		BarbaBuffer requestDataBuf(decodeBuffer.data(), decodeBuffer.size());
		this->Crypt(&requestDataBuf, 0, false);
		requestDataBuf.append((BYTE)0);
		requestDataBuf.append((BYTE)0);
		std::string ret = (char*)requestDataBuf.data(); //should be ANSI
		return ret;
	}
	catch (...)
	{
	}

	return _T("");
}

void BarbaCourierHttp::InitRequestVars(std::tstring& src, LPCTSTR fileName, LPCTSTR contentType, size_t transferSize, size_t fileHeaderSize, bool outgoing)
{
	if (fileName==NULL) fileName = _T("");
	if (contentType==NULL) contentType = _T("");

	//host
	if (!this->CreateStruct.HostName.empty())
	{
		BarbaUtils::UpdateHttpRequest(&src, _T("Host"), this->CreateStruct.HostName);
		BarbaUtils::UpdateHttpRequest(&src, _T("Origin"), this->CreateStruct.HostName);
	}

	//filename
	StringUtils::ReplaceAll(src, _T("{filename}"), fileName);
	StringUtils::ReplaceAll(src, _T("{filetitle}"), BarbaUtils::GetFileTitleFromUrl(fileName));
	StringUtils::ReplaceAll(src, _T("{fileextension}"), BarbaUtils::GetFileExtensionFromUrl(fileName));

	//fileSize
	TCHAR fileSizeStr[20];
	_ltot_s((long)transferSize, fileSizeStr, 10);
	BarbaUtils::UpdateHttpRequest(&src, _T("Content-Length"), fileSizeStr);

	//time
	std::tstring curTime = BarbaUtils::FormatTimeForHttp();
	BarbaUtils::UpdateHttpRequest(&src, _T("Date"), curTime);
	BarbaUtils::UpdateHttpRequest(&src, _T("Last-Modified"), curTime);

	//contentType
	BarbaUtils::UpdateHttpRequest(&src, _T("Content-Type"), contentType);

	//prepare RequestData: 
	CHAR other[100] = {0};
	sprintf_s(other, "fileHeaderSize:%u", fileHeaderSize);
	std::tstring data = CreateRequestParam(transferSize, outgoing, other);

	StringUtils::ReplaceAll(src, _T("{data}"), data);
}
