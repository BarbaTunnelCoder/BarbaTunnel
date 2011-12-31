#pragma once
#include "General.h"

class BarbaUtils
{
public:
	//@param folder buffer with MAX_PATH length
	static void GetModuleFolder(TCHAR* folder);
	static bool GetPortRange(LPCTSTR value, u_short* startPort, u_short* endPort);
	static bool GetProtocolAndPort(LPCTSTR value, BYTE* protocol, u_short* port);
	//@return number of bytes copied to buffer
	static void ConvertHexStringToBuffer(LPCTSTR hexString, std::vector<BYTE>* buffer);
	static std::tstring ConvertBufferToHexString(std::vector<BYTE>* buffer);
	// @param lphProcess return handle to opened process; if not NULL user must close handle after use it
	static bool SimpleShellExecute(LPCTSTR fileName, LPCTSTR commandLine=_T(""), int nShow=SW_SHOWNORMAL, LPCTSTR lpszWorkDirectory = NULL, LPCTSTR lpVerb=NULL, HWND hWnd=NULL, DWORD* lpExitCode=NULL);
	static bool SimpleShellExecuteAndWait(LPCTSTR fileName, LPCTSTR commandLine=_T(""), int nShow=SW_SHOWNORMAL, LPCTSTR lpszWorkDirectory = NULL, LPCTSTR lpVerb=NULL, HWND hWnd=NULL);
	//@param value: StartPort-EndPort,StartPort-EndPort,StartPort-EndPort
	//return number of parsed portRanges
	static void ParsePortRanges(LPCTSTR value, std::vector<PortRange>* portRange);
	static bool IsThreadAlive(const HANDLE hThread, bool* alive);
	static bool LoadFileToBuffer(LPCTSTR fileName, std::vector<BYTE>* buffer);
	static std::string LoadFileToString(LPCTSTR fileName);
	static u_int GetRandom(u_int start, u_int end);
	static void FindFiles(LPCTSTR folder, LPCTSTR search, std::vector<std::tstring>* files);
	static std::tstring FindFileName(LPCTSTR filePath);
	static std::tstring GetFileExtensionFromUrl(LPCTSTR url);
	static std::tstring GetFileNameFromUrl(LPCTSTR url);
	static std::tstring GetFileTitleFromUrl(LPCTSTR url);
	static std::tstring GetFileUrlFromHttpRequest(LPCTSTR httpRequest);
	static std::tstring GetKeyValueFromString(LPCTSTR httpRequest, LPCTSTR key);
	static std::tstring FormatTimeForHttp();
	static std::tstring FormatTimeForHttp(time_t* t);
};