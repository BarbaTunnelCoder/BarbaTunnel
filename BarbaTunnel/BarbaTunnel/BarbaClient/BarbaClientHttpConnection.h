#pragma once
#include "BarbaClientTcpConnectionBase.h"
#include "BarbaCourierHttpClient.h"

class BarbaClientHttpConnection : public BarbaClientTcpConnectionBase
{
public:
	class Courier : public BarbaCourierHttpClient
	{
	public:
		explicit Courier(BarbaClientHttpConnection* connection, CreateStrcutHttp* cs);
		virtual ~Courier(void);
		void Receive(BarbaBuffer* data) override;
		void Crypt(BYTE* data, size_t dataSize, size_t index, bool encrypt) override;

	private:
		void GetFakeFile(TCHAR* filename, std::tstring* contentType, size_t* fileSize, BarbaBuffer* fakeFileHeader, bool createNew) override;
		std::tstring GetHttpPostTemplate(bool bombardMode) override;
		std::tstring GetHttpGetTemplate(bool bombardMode) override;
		BarbaClientHttpConnection* _Connection;
	};

public:
	explicit BarbaClientHttpConnection(BarbaClientConfig* config);
	virtual ~BarbaClientHttpConnection(void);
	void Init() override;
};


