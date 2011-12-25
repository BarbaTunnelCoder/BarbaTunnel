#include "stdafx.h"
#include "General.h"
#include "BarbaUtils.h"

void BarbaUtils::GetModuleFolder(TCHAR* folder)
{
	::GetModuleFileName(NULL, folder, MAX_PATH);
	for (int i=_tcsclen(folder); i>=0; i--)
	{
		if (folder[i]=='\\' || folder[i]=='/')
		{
			folder[i] = 0;
			break;
		}
		folder[i] = 0;
	}
}

bool BarbaUtils::GetPortRange(LPCTSTR value, u_short* startPort, u_short* endPort)
{
	//VirtualIpRange
	TCHAR* dash = _tcschr((TCHAR*)value, '-');
	
	TCHAR ipBuffer[100];
	_tcsncpy_s(ipBuffer, _countof(ipBuffer), value, dash!=NULL ? dash-value : _tcslen(value));
	*startPort = (u_short)_tcstoul(ipBuffer, NULL, 0);
	*endPort = *startPort; //default
	if (dash!=NULL)
	{
		_tcscpy_s(ipBuffer, _countof(ipBuffer), dash+1);
		*endPort = (u_short)_tcstoul(ipBuffer, NULL, 0);
	}


	return *startPort!=0 && (*endPort-*startPort)>=0;
}

bool BarbaUtils::GetProtocolAndPort(LPCTSTR value, BYTE* protocol, u_short* port)
{
	bool ret = false;
	*protocol = 0;
	*port = 0;
	TCHAR buffer[100];
	_tcscpy_s(buffer, value);

	TCHAR* currentPos = NULL;
	TCHAR* token = _tcstok_s(buffer, _T(":"), &currentPos);
		
	int index = 0;
	while (token!=NULL && index<2)
	{
		if (index==0 && _tcsicmp(token, _T("*"))==0) {*protocol = 0; ret = true;}
		else if (index==0) {*protocol = PacketHelper::ConvertStringProtocol(token); ret = *protocol!=0;}
		if (index==1) *port = (u_short)_tcstoul(token, NULL, 0);
		token = _tcstok_s(NULL, _T("."), &currentPos);
		index++;
	}

	return ret;
}

void BarbaUtils::ConvertHexStringToBuffer(LPCTSTR hexString, SimpleBuffer* buf)
{
	int len = _tcslen(hexString);
	buf->New(len/2);

	int bufferIndex = 0;
	for(int i=0; i<(int)buf->GetSize(); i+=2)
	{
		char b[3];
		b[0] = hexString[i];
		b[1] = hexString[i+1];
		b[2] = 0;
		buf->GetData()[bufferIndex++] = (BYTE)strtol(b, NULL, 16);
	}
}

bool BarbaUtils::SimpleShellExecuteAndWait(LPCTSTR fileName, LPCTSTR commandLine, int nShow, LPCTSTR lpszWorkDirectory, LPCTSTR lpVerb, HWND hWnd)
{
	DWORD exitCode;
	return SimpleShellExecute(fileName, commandLine, nShow, lpszWorkDirectory, lpVerb, hWnd, &exitCode);
}


bool BarbaUtils::SimpleShellExecute(LPCTSTR fileName, LPCTSTR commandLine, int nShow, LPCTSTR lpszWorkDirectory, LPCTSTR lpVerb, HWND hWnd, DWORD* lpExitCode)
{
	SHELLEXECUTEINFO s;
	memset (&s,0,sizeof s);

	s.cbSize = sizeof SHELLEXECUTEINFO;
	s.fMask = (lpExitCode!=NULL) ? SEE_MASK_NOCLOSEPROCESS : 0;
	s.hwnd = hWnd;
	s.lpVerb = lpVerb;
	s.lpFile = fileName;
	s.lpParameters = commandLine;
	s.lpDirectory = lpszWorkDirectory;
	s.nShow = nShow;
	bool ret = ShellExecuteEx(&s) != FALSE;

	if (lpExitCode!=NULL)
	{
		for(;;)
		{
			WaitForSingleObject(s.hProcess, INFINITE);
			GetExitCodeProcess(s.hProcess, lpExitCode);
			if ( *lpExitCode!=STILL_ACTIVE )
				break;
		}
	}

	//close process handle
	if (s.hProcess!=NULL)
		CloseHandle(s.hProcess);

	return ret;
}

size_t BarbaUtils::ParsePortRanges(LPCTSTR value, PortRange* portRanges, size_t portRangeCount)
{
	size_t count = 0;
	
	TCHAR buffer[1000];
	_tcscpy_s(buffer, value);
	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(buffer, _T(","), &currentPos);
		
	while (token!=NULL && count<portRangeCount)
	{
		PortRange* portRange = &portRanges[count];
		if (BarbaUtils::GetPortRange(token, &portRange->StartPort, &portRange->EndPort))
		{
			count++;
		}
		token = _tcstok_s(NULL, _T(","), &currentPos);
	}

	return count;

}

bool BarbaUtils::IsThreadAlive(const HANDLE hThread, bool* alive)
{
	DWORD dwExitCode = 0;
	if (!GetExitCodeThread(hThread, &dwExitCode))
		return false;
	*alive = dwExitCode==STILL_ACTIVE;
	return true;
}

bool BarbaUtils::LoadFileToBuffer(LPCTSTR fileName, SimpleBuffer* buffer)
{
	bool ret = false;

	FILE* f;
	if (_tfopen_s(&f, fileName, _T("rb"))!=0)
		return false;

	fseek(f, 0, SEEK_END);
	size_t fileSize = ftell(f); 
	fseek(f, 0, SEEK_SET);

	buffer->New(fileSize);
	ret = fread_s(buffer->GetData(), buffer->GetSize(), 1, fileSize, f)==fileSize;
	fclose(f);
	return ret;
}

std::string BarbaUtils::LoadFileToString(LPCTSTR fileName)
{
	std::ifstream ifs(fileName);
	std::string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	return str;
}

u_int BarbaUtils::GetRandom(u_int start, u_int end)
{
	u_int range = end - start + 1;

	u_int number;
	if ( rand_s(&number) )
		_tprintf_s(_T("rand_s return error!\n"));

	return (u_int)((double)number / ((double) UINT_MAX + 1 ) * range) + start;
}

void BarbaUtils::FindFiles(LPCTSTR folder, LPCTSTR search, std::vector<std::tstring>* files)
{
	TCHAR file[MAX_PATH];
	WIN32_FIND_DATA findData = {0};
		
	//findData.
	_stprintf_s(file, _countof(file), _T("%s\\%s"), folder, search);
	HANDLE findHandle = FindFirstFile(file, &findData);
	BOOL bfind = findHandle!=NULL;
	while (bfind)
	{
		TCHAR fullPath[MAX_PATH] = {0};
		_stprintf_s(fullPath, _T("%s\\%s"), folder, findData.cFileName);
		files->push_back(fullPath);
		bfind = FindNextFile(findHandle, &findData);
	}
	FindClose(findHandle);
}

std::tstring BarbaUtils::FindFileName(LPCTSTR filePath)
{
	int len = _tcslen(filePath);
	int start = 0;
	int end = 0;
	for (int i=len-1; i>=0; i--)
	{
		if (filePath[i]=='\\' || filePath[i]=='/')
		{
			start = i + 1;
			break;
		}

		if (filePath[i]=='.')
			end = i;
	}

	if (end<=start)
		end = len;

	TCHAR fileName[MAX_PATH];
	_tcsncpy_s(fileName, &filePath[start], end-start);

	return fileName;
}

std::tstring BarbaUtils::GetFileUrlFromHttpRequest(LPCTSTR httpRequest)
{
	std::tstring  request = httpRequest;
	StringUtils::ReplaceAll(request, _T("\r"), _T(" "));
	StringUtils::ReplaceAll(request, _T("\n"), _T(" "));

	std::vector<std::tstring> tokenize;
	StringUtils::Tokenize(request.data(), _T(" "), &tokenize);
	return tokenize.size()>=2 ? tokenize[1] : _T("");	
}

std::tstring BarbaUtils::GetFileNameFromUrl(LPCTSTR url)
{
	int urlLen = _tcslen(url);
	LPCTSTR endP = _tcsrchr(url, '?');
	ptrdiff_t end = endP!=NULL ? endP-url : urlLen ;
	
	//remove query string
	TCHAR path[MAX_PATH];
	_tcsncpy_s(path, url, min(end,MAX_PATH));
	int pathLen = _tcslen(path);

	//find last last
	LPCTSTR startP = _tcsrchr(path, '/');
	if (startP==NULL) startP = _tcsrchr(path, '\\');
	ptrdiff_t start = startP!=NULL ? startP-path + 1 : 0;

	TCHAR fileName[MAX_PATH];
	_tcsncpy_s(fileName, path + start, pathLen - start);

	return fileName;
}

std::tstring BarbaUtils::GetFileExtensionFromUrl(LPCTSTR url)
{
	std::tstring fileName = GetFileNameFromUrl(url);
	LPCTSTR startP = _tcsrchr(fileName.data(), '.');
	return startP==NULL ? _T("") : startP+1;
}

std::tstring BarbaUtils::GetFileTitleFromUrl(LPCTSTR url)
{
	std::tstring fileNameStr = GetFileNameFromUrl(url);
	LPCTSTR fileName = fileNameStr.data();
	LPCTSTR endP = _tcsrchr(fileName, '.');
	ptrdiff_t end = endP!=NULL ? endP-fileName : _tcslen(fileName);
	TCHAR fileTitle[MAX_PATH];
	_tcsncpy_s(fileTitle, fileName, end);
	return fileTitle;
}

std::tstring BarbaUtils::GetKeyValueFromHttpRequest(LPCTSTR httpRequest, LPCTSTR key)
{
	CHAR phrase[BARBA_MaxKeyName+2];
	_stprintf_s(phrase, "%s=", key);
	const TCHAR* start = _tcsstr(httpRequest, key);
	if (start==NULL)
		return _T("");
	start = start + _tcslen(phrase);

	const TCHAR* end = _tcsstr(start, ";");
	if (end==NULL) end = _tcsstr(start, "\r");
	if (end==NULL) end = httpRequest + _tcslen(httpRequest);

	TCHAR buffer[255];
	strncpy_s(buffer, start, end-start);
	return buffer;
}

