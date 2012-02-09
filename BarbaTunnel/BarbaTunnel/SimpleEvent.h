#pragma once

class SimpleEvent
{
private:
	HANDLE EventHandle;

public:
	explicit SimpleEvent(bool manualReset, bool initState)
	{
		this->EventHandle = CreateEvent(NULL, manualReset, initState, NULL);
	}

	explicit SimpleEvent(HANDLE eventHandle)
	{
		this->EventHandle = NULL;
		Attach(eventHandle);
	}

	explicit SimpleEvent()
	{
		this->EventHandle = NULL;
	}

	void Attach(HANDLE eventHandle)
	{
		if (this->EventHandle!=NULL)
			throw _T("SimpleEvent could not Attach when already contain handle!");
		this->EventHandle = eventHandle;
	}

	HANDLE Detach()
	{
		HANDLE ret = this->EventHandle;
		this->EventHandle = NULL;
		return ret;
	}

	void Close()
	{
		if (this->EventHandle!=NULL)
		{
			CloseHandle(this->EventHandle);
			this->EventHandle = NULL;
		}
	}

	~SimpleEvent()
	{
		Close();
	}

	bool IsSet()
	{
		return Wait(0)==WAIT_OBJECT_0;
	}

	void Set()
	{
		SetEvent(this->EventHandle);
	}

	void Reset()
	{
		ResetEvent(this->EventHandle);
	}

	DWORD Wait(DWORD milliseconds)
	{
		return WaitForSingleObject(this->EventHandle, milliseconds);
	}

	HANDLE GetHandle() 
	{
		return this->EventHandle;
	}
};