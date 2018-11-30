#pragma once
#include "BarbaBuffer.h"

class BarbaCourierDatagram
{
public:
	static const u_int MaxMessageChunksCount = 10000;
	static const u_int MaxMessagesCount = 100000; //100K messages!
	static const u_int MaxDataControlCount = 1000; //100 queue

	struct CreateStrcut
	{
		CreateStrcut() { MaxChunkSize=1400; MessageTimeout=10000; RemoteIp=0;}
		size_t MaxChunkSize;
		DWORD MessageTimeout;
		DWORD RemoteIp;
	};

private:
	class Message
	{
	public:
		Message(DWORD id, DWORD totalChunks);
		Message(DWORD id, BarbaBuffer* data, DWORD maxChunkSize);
		Message(DWORD id, BYTE* data, size_t dataSize, DWORD maxChunkSize);
		void AddChunk(DWORD chunkIndex, BYTE* data, size_t dataSize);
		bool IsCompleted(); //return true if message contains all chunks
		void GetData(BarbaBuffer* data); //merge all chunks and return data
		~Message();

		DWORD Id; 
		DWORD LastUpdateTime;
		BarbaArray<BarbaBuffer*> Chunks;

	private:
		void Construct(DWORD id, BYTE* data, size_t dataSize, DWORD maxChunkSize);
		DWORD AddedChunksCount;
	};

	//data control
	class DataControlManager
	{
	public:
		DataControlManager();
		~DataControlManager();
		bool IsDataControl(BarbaBuffer* data);
		bool CheckDataControl(BarbaBuffer* data);
		void Process(bool force);
		void Send(BarbaBuffer* data);
		void SendAck(DWORD id);
		void Remove(DWORD id);
		BarbaCourierDatagram* Courier;

	private:
		DWORD NextId;
		DWORD LastSentId;
		DWORD LastReceivedId;
		BarbaList<BarbaBuffer*> Datas;
		DWORD LastSentTime;
	};
	DataControlManager DataControlManager;

public:
	explicit BarbaCourierDatagram(CreateStrcut* cs);
	virtual ~BarbaCourierDatagram(void);
	void Log2(LPCTSTR format, ...);
	void Log3(LPCTSTR format, ...);
	virtual void SendData(BarbaBuffer* data); //end-user send data with this method
	virtual void SendDataControl(BarbaBuffer* data);
	virtual void Encrypt(BYTE* data, size_t dataSize, size_t index)=0;
	virtual void Decrypt(BYTE* data, size_t dataSize, size_t index)=0;
	virtual void Init();
	DWORD GetSessionId() { return _SessionId; }
	CreateStrcut* GetCreateStruct() {return _CreateStruct;}

protected:
	virtual void ReceiveData(BarbaBuffer* data); 
	virtual void ReceiveDataControl(BarbaBuffer* data);
	virtual bool PreReceiveData(BarbaBuffer* data);
	virtual bool PreReceiveDataControl(BarbaBuffer* data);
	virtual void Timer(); //should always call base class member

	void LogImpl(int level, LPCTSTR format, va_list _ArgList);
	DWORD GetNewMessageId();
	void SendChunkToInbound(BarbaBuffer* data); //Subclasser should called it when got new chunk
	virtual void SendChunkToOutbound(BarbaBuffer* chunk)=0; //Subclasser should send chunk
	DWORD _SessionId;
	void TimerThread();

private:
	DWORD LastTimerTime;
	void DoTimer();
	CreateStrcut* _CreateStruct;
	DWORD LastMessageId;
	BarbaArray<Message*> Messages;
	void RemoveMessage(int messageIndex);
	void RemoveTimeoutMessages();
	DWORD LastCleanTimeoutMessagesTime;
};

