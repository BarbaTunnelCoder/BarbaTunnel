#pragma once
#include "General.h"

class BarbaUtils
{
public:
	//@param folder buffer with MAX_PATH length
	static std::tstring GetModuleFolder();
	static bool GetPortRange(LPCTSTR value, u_short* startPort, u_short* endPort);
	static bool GetProtocolAndPort(LPCTSTR value, BYTE* protocol, u_short* port);
	static void GetProtocolAndPortArray(LPCTSTR value, std::vector<ProtocolPort>* result);
	//@return number of bytes copied to buffer
	static void ConvertHexStringToBuffer(LPCTSTR hexString, BarbaBuffer* buffer);
	static std::tstring ConvertBufferToHexString(BarbaBuffer* buffer);
	// @param lphProcess return handle to opened process; if not NULL user must close handle after use it
	static bool SimpleShellExecute(LPCTSTR fileName, LPCTSTR commandLine=_T(""), int nShow=SW_SHOWNORMAL, LPCTSTR lpszWorkDirectory = NULL, LPCTSTR lpVerb=NULL, HWND hWnd=NULL, DWORD* lpExitCode=NULL);
	static bool SimpleShellExecuteAndWait(LPCTSTR fileName, LPCTSTR commandLine=_T(""), int nShow=SW_SHOWNORMAL, LPCTSTR lpszWorkDirectory = NULL, LPCTSTR lpVerb=NULL, HWND hWnd=NULL);
	static bool IsThreadAlive(const HANDLE hThread, bool* alive);
	static bool LoadFileToBuffer(LPCTSTR fileName, BarbaBuffer* buffer);
	static std::string LoadFileToString(LPCTSTR fileName);
	static u_int GetRandom(u_int start, u_int end);
	static void FindFiles(LPCTSTR folder, LPCTSTR search, std::vector<std::tstring>* files);
	static void FindFiles(LPCTSTR folder, LPCTSTR search, bool recursive, std::vector<std::tstring>* files);
	static std::tstring FindFileTitle(LPCTSTR filePath);
	static std::tstring GetFileExtensionFromUrl(LPCTSTR url);
	static std::tstring GetFileNameFromUrl(LPCTSTR url);
	static std::tstring GetFileTitleFromUrl(LPCTSTR url);
	static std::tstring GetFileFolderFromUrl(LPCTSTR url);
	static std::tstring GetFileUrlFromHttpRequest(LPCTSTR httpRequest);
	static std::tstring GetKeyValueFromString(LPCTSTR str, LPCTSTR key);
	static int GetKeyValueFromString(LPCTSTR str, LPCTSTR key, int defValue);
	static void SetKeyValue(std::tstring* str, LPCTSTR key, LPCTSTR value);
	static void SetKeyValue(std::tstring* str, LPCTSTR key, int value);
	static std::tstring FormatTimeForHttp();
	static std::tstring FormatTimeForHttp(time_t* t);
	static std::tstring ConvertIpToString(u_int ip, bool anonymously);
	static bool IsFileExists(LPCTSTR filename);
	static void UpdateHttpRequest(std::tstring* httpRequest, std::tstring key, std::tstring value);
	static void UpdateHttpRequest(std::tstring* httpRequest, LPCTSTR host, LPCTSTR fileName, LPCTSTR contentType, size_t contentLength, LPCTSTR data);
	static std::string PrepareHttpRequest(std::tstring request);
	static std::tstring GetTimeString(int timeZone);
	static std::tstring GetTimeString(time_t _Time, int timeZone);
	//@return -1 if not valid otherwise is the time zone different in second
	static int GetTimeZoneFromString(LPCTSTR timeZone);
};