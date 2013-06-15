// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#endif // WIN32_LEAN_AND_MEAN
#define _CRT_RAND_S

#include "targetver.h"

#include <windows.h>
#include <winsock2.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include <process.h>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <Iphlpapi.h>
#include <in6addr.h>
#include "WinpkFilter\Common.h"
#include "WinpkFilter\iphlp.h"

//UNICODE support
namespace std
{
#ifdef _UNICODE
typedef wstring tstring;
typedef wstringstream tstringstream;
#else
typedef string tstring;
typedef stringstream tstringstream;
#endif
}

// memory leak detection ** dont forget call : _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#ifdef _DEBUG
    #define new new( _CLIENT_BLOCK, __FILE__, __LINE__)
#endif // _DEBUG

#pragma comment(lib, "Ws2_32.lib")