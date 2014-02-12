#pragma once
#include "windivert.h"

class WinDivertApi
{
public:
	HMODULE ModuleHandle;
	typedef BOOL (*WINDIVERTCLOSE)(HANDLE handle);
	typedef HANDLE(*WINDIVERTOPEN)(const char *filter, WINDIVERT_LAYER layer, INT16 priority, UINT64 flags);
	typedef BOOL(*WINDIVERTRECV)(HANDLE handle, PVOID pPacket, UINT packetLen, PWINDIVERT_ADDRESS pAddr, UINT *readLen);
	typedef BOOL(*WINDIVERTSEND)(HANDLE handle, PVOID pPacket, UINT packetLen, PWINDIVERT_ADDRESS pAddr, UINT *writeLen);
	WINDIVERTOPEN Open;
	WINDIVERTCLOSE Close;
	WINDIVERTRECV Recv;
	WINDIVERTSEND Send;

	WinDivertApi()
	{
		this->ModuleHandle = NULL;
	}

	void Init(HMODULE moudle)
	{
		this->ModuleHandle = moudle;
		this->Open = (WINDIVERTOPEN)GetFunction("WinDivertOpen");
		this->Close = (WINDIVERTCLOSE)GetFunction("WinDivertClose");
		this->Recv = (WINDIVERTRECV)GetFunction("WinDivertRecv");
		this->Send = (WINDIVERTSEND)GetFunction("WinDivertSend");
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
