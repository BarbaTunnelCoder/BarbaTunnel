#pragma once
#include "divert.h"

class WinDivertApi
{
public:
	HMODULE ModuleHandle;
	typedef BOOL (__stdcall *DIVERTCLOSE)(HANDLE handle);
	typedef HANDLE (__stdcall *DIVERTOPEN)(const char *filter, DIVERT_LAYER layer, INT16 priority, UINT64 flags);
	typedef BOOL (__stdcall *DIVERTRECV)(HANDLE handle, PVOID pPacket, UINT packetLen, PDIVERT_ADDRESS pAddr, UINT *readLen);
	typedef BOOL (__stdcall *DIVERTSEND)(HANDLE handle, PVOID pPacket, UINT packetLen, PDIVERT_ADDRESS pAddr, UINT *writeLen);
	DIVERTOPEN DivertOpen;
	DIVERTCLOSE DivertClose;
	DIVERTRECV DivertRecv;
	DIVERTSEND DivertSend;

	WinDivertApi()
	{
		this->ModuleHandle = NULL;
	}

	void Init(HMODULE moudle)
	{
		this->ModuleHandle = moudle;
		this->DivertOpen = (DIVERTOPEN)GetFunction("DivertOpen");
		this->DivertClose = (DIVERTCLOSE)GetFunction("DivertClose");
		this->DivertRecv = (DIVERTRECV)GetFunction("DivertRecv");
		this->DivertSend = (DIVERTSEND)GetFunction("DivertSend");
	}

private:
	FARPROC GetFunction(LPCSTR procName)
	{
		FARPROC ret = GetProcAddress(ModuleHandle, procName);
		if (procName==NULL) 
			throw _T("Could not find of the WinDivert Functions!");
		return ret;
	}
};
