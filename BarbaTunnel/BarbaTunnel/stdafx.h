// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif // WIN32_LEAN_AND_MEAN

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
#include <iostream>
#include "WinpkFilter\Common.h"
#include "WinpkFilter\ndisapi.h"
#include "WinpkFilter\iphlp.h"

// memory leak detection ** dont forget call : _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#ifdef _DEBUG
    #define new new( _CLIENT_BLOCK, __FILE__, __LINE__)
#endif // _DEBUG