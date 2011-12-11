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

	~SimpleEvent()
	{
		CloseHandle(this->EventHandle);
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
};