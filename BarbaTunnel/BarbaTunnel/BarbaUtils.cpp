#include "stdafx.h"
#include "General.h"
#include "BarbaUtils.h"

std::tstring BarbaUtils::GetModuleFolder()
{
	TCHAR file[MAX_PATH];
	::GetModuleFileName(NULL, file, MAX_PATH);
	return GetFileFolderFromUrl(file);
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

void BarbaUtils::GetProtocolAndPortArray(LPCTSTR value, std::vector<ProtocolPort>* result)
{
	TCHAR buffer[1000];
	_tcscpy_s(buffer, value);

	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(buffer, _T(","), &currentPos);
		
	while (token!=NULL)
	{
		ProtocolPort protocol;
		if (BarbaUtils::GetProtocolAndPort(token, &protocol.Protocol, &protocol.Port))
		{
			result->push_back(protocol);
		}
		token = _tcstok_s(NULL, _T(","), &currentPos);
	}
}

void BarbaUtils::ConvertHexStringToBuffer(LPCTSTR hexString, BarbaBuffer* buffer)
{
	size_t len = _tcslen(hexString);
	buffer->assign(len/2);

	for(size_t i=0; i<buffer->size(); i++)
	{
		char b[3];
		b[0] = hexString[i*2];
		b[1] = hexString[i*2+1];
		b[2] = 0;
		buffer->data()[i] = (BYTE)strtol(b, NULL, 16);
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

void BarbaUtils::ParsePortRanges(LPCTSTR value, std::vector<PortRange>* portRanges)
{
	BarbaBuffer sbuffer((BYTE*)value, _tcslen(value)*2+2);
	std::tstring buffer = value;
	if (buffer.empty())
		return;

	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(&buffer.front(), _T(","), &currentPos);
		
	while (token!=NULL)
	{
		PortRange portRange;
		if (BarbaUtils::GetPortRange(token, &portRange.StartPort, &portRange.EndPort))
		{
			portRanges->push_back(portRange);
		}
		token = _tcstok_s(NULL, _T(","), &currentPos);
	}
}

bool BarbaUtils::IsThreadAlive(const HANDLE hThread, bool* alive)
{
	DWORD dwExitCode = 0;
	if (!GetExitCodeThread(hThread, &dwExitCode))
		return false;
	*alive = dwExitCode==STILL_ACTIVE;
	return true;
}

bool BarbaUtils::LoadFileToBuffer(LPCTSTR fileName, BarbaBuffer* buffer)
{
	bool ret = false;

	FILE* f;
	if (_tfopen_s(&f, fileName, _T("rb"))!=0)
		return false;

	fseek(f, 0, SEEK_END);
	size_t fileSize = ftell(f); 
	fseek(f, 0, SEEK_SET);

	buffer->assign(fileSize);
	ret = fread_s(buffer->data(), buffer->size(), 1, fileSize, f)==fileSize;
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
	FindFiles(folder, search, false, files);
}

void BarbaUtils::FindFiles(LPCTSTR folder, LPCTSTR search, bool recursive, std::vector<std::tstring>* files)
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
		if (_tcslen(findData.cFileName)>0 && _tcscmp(findData.cFileName, _T("."))!= 0 && _tcscmp(findData.cFileName, _T(".."))!= 0)
		{
			_stprintf_s(fullPath, _T("%s\\%s"), folder, findData.cFileName);
			files->push_back(fullPath);
		}
		bfind = FindNextFile(findHandle, &findData);
	}
	FindClose(findHandle);

	if (recursive)
	{
		_stprintf_s(file, _countof(file), _T("%s\\*"), folder);
		HANDLE findHandle = FindFirstFile(file, &findData);
		BOOL bfind = findHandle!=NULL;
		while (bfind)
		{
			TCHAR fullPath[MAX_PATH] = {0};
			if( (findData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0 && (findData.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN)==0 && (findData.dwFileAttributes& FILE_ATTRIBUTE_SYSTEM)==0 && 
				_tcscmp(findData.cFileName, _T("."))!= 0 && _tcscmp(findData.cFileName, _T(".."))!= 0)
			{
				_stprintf_s(fullPath, _T("%s\\%s"), folder, findData.cFileName);
				FindFiles(fullPath, search, recursive, files);
			}
			bfind = FindNextFile(findHandle, &findData);
		}
		FindClose(findHandle);
	}
}

std::tstring BarbaUtils::FindFileTitle(LPCTSTR filePath)
{
	size_t len = _tcslen(filePath);
	size_t start = 0;
	size_t end = 0;
	for (size_t i=len-1; i>=0; i--)
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

	TCHAR fileTitle[MAX_PATH];
	_tcsncpy_s(fileTitle, &filePath[start], end-start);

	return fileTitle;
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
	size_t urlLen = _tcslen(url);
	LPCTSTR endP = _tcsrchr(url, '?');
	ptrdiff_t end = endP!=NULL ? endP-url : urlLen ;
	
	//remove query string
	TCHAR path[MAX_PATH];
	_tcsncpy_s(path, url, min(end,MAX_PATH-1));
	size_t pathLen = _tcslen(path);

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
	LPCTSTR startSlash = _tcsrchr(fileName.data(), '/');
	return startP==NULL || startSlash>startP ? _T("") : startP+1;
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

std::tstring BarbaUtils::GetFileFolderFromUrl(LPCTSTR url)
{
	LPCTSTR endP = _tcsrchr(url, '\\');
	if (endP==NULL) endP = _tcsrchr(url, '/');
	if (endP==NULL)
		return _T("");
	ptrdiff_t end = endP-url;
	TCHAR fileTitle[MAX_PATH];
	_tcsncpy_s(fileTitle, url, end);
	return fileTitle;
}

std::tstring BarbaUtils::GetKeyValueFromString(LPCTSTR httpRequest, LPCTSTR key)
{
	CHAR phrase[BARBA_MaxKeyName+2];
	_stprintf_s(phrase, "%s=", key);
	const TCHAR* start = _tcsstr(httpRequest, key);
	if (start==NULL)
		return _T("");
	start = start + _tcslen(phrase);

	const TCHAR* end = _tcsstr(start, ";");
	if (end==NULL) end = _tcsstr(start, "\r");
	if (end==NULL) end = _tcsstr(start, "\n");
	if (end==NULL) end = httpRequest + _tcslen(httpRequest);

	TCHAR buffer[255];
	size_t maxValueLen  = min(end-start, _countof(buffer)-1);
	_tcsncpy_s(buffer, start, maxValueLen);
	return buffer;
}

u_int BarbaUtils::GetKeyValueFromString(LPCTSTR httpRequest, LPCTSTR key, u_int defValue)
{
	std::tstring valueStr = BarbaUtils::GetKeyValueFromString(httpRequest,key);
	u_int fileSize = valueStr.empty() ? defValue : _tcstoul(valueStr.data(), 0, 0);
	return fileSize;
}


std::tstring BarbaUtils::FormatTimeForHttp()
{
	time_t t;
	time(&t);
	return FormatTimeForHttp(&t);
}

std::tstring BarbaUtils::FormatTimeForHttp(time_t* t)
{
	TCHAR buf[200];

	struct tm timeinfo ;
	gmtime_s( &timeinfo, t );
	_tcsftime(buf, sizeof buf, _T("%a, %d %b %Y %H:%M:%S GMT"), &timeinfo);
	return buf;
}

void BarbaUtils::UpdateHttpRequest(std::tstring* httpRequest, std::tstring key, std::tstring value)
{
	value.insert(0, " ");
	key.push_back(':');

	size_t start_pos = 0;
	start_pos = httpRequest->find(key, start_pos);
	if (start_pos!=std::string::npos)
	{
		start_pos = start_pos + key.size();
		size_t end_pos = httpRequest->find('\r', start_pos);
		if (end_pos==std::string::npos) end_pos = httpRequest->find('\n', start_pos);
		if (end_pos==std::string::npos) end_pos = httpRequest->size();
		httpRequest->replace(start_pos, end_pos-start_pos, value);
	}
}

std::tstring BarbaUtils::ConvertIpToString(u_int ip)
{
	TCHAR buf[100];
	PacketHelper::ConvertIpToString(ip, buf, _countof(buf));
	return buf;
}

bool BarbaUtils::IsFileExists(LPCTSTR filename)
{    
	FILE* f;
	if (_tfopen_s(&f, filename, _T("rb"))!=0)
		return false;
	fclose(f);
	return true;    
}