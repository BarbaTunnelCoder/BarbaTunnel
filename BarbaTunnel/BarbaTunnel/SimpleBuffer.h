#pragma once

class SimpleBuffer
{
public:
	explicit SimpleBuffer()
	{
		Reset();
	}

	explicit SimpleBuffer(size_t size)
	{
		Reset();
		New(size);
	}

	explicit SimpleBuffer(BYTE* buffer, size_t size)
	{
		CopyFromBuffer(buffer, size);
	}

	void New(size_t size)
	{
		if (this->Buffer!=NULL)
			delete this->Buffer;
		
		Reset();
		if (size>0)
		{
			this->Buffer = new BYTE[size];
			this->BufferSize = size;
			memset(this->Buffer, 0, size);
		}
	}

	void Attach(BYTE* buffer, size_t size)
	{
		if (this->Buffer!=NULL)
			delete this->Buffer;
		this->Buffer = buffer;
		this->BufferSize = size;
	}

	void Detach()
	{
		Reset();
	}

	~SimpleBuffer(void)
	{
		if (this->Buffer!=NULL)
			delete this->Buffer;
	}

	BYTE* GetData() {return this->Buffer;}
	size_t GetSize() {return this->BufferSize;}

private:
	SimpleBuffer(const SimpleBuffer& src); //no copy constructor
	SimpleBuffer operator=(const SimpleBuffer& src); //no copy operator 

	void Reset()
	{
		this->Buffer = NULL;
		this->BufferSize = 0;
	}

	void CopyFromBuffer(BYTE* buffer, size_t size)
	{
		Reset();
		if (size>0)
		{
			this->Buffer = new BYTE[size];
			memcpy_s(this->Buffer, size, buffer, size);
			this->BufferSize = size;
		}
	}
	BYTE* Buffer;
	size_t BufferSize;
};

