#include "stdafx.h"
#include "BarbaClient\BarbaClientApp.h"
#include "BarbaServer\BarbaServerApp.h"
#include "WinpkFilterDriver.h"
#include "WinDivertFilterDriver.h"

BarbaFilterDriver* CreateFilterDriverByName(LPCTSTR name)
{
	if (_tcsicmp(name, _T("WinpkFilter"))==0)
		return new WinpkFilterDriver();

	if (_tcsicmp(name, _T("WinDivert"))==0)
		return new WinDivertFilterDriver();

	throw new BarbaException(_T("Invalid FilterDriver (%s)!"), name);
}

void InitMemoryLeackReport(_HFILE fileHandle)
{
	UNREFERENCED_PARAMETER(fileHandle); //for release mode

	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, fileHandle );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, fileHandle );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, fileHandle );
}

void test()
{
	BarbaBuffer buf(50, 1);
	printf("\nsize: %d, capacity:%d\n", buf.size(), buf.capacity());
	for (int i=0; i<buf.size(); i++)
		printf("%x,", buf[i]);

	BarbaBuffer buf2(5, 2);
	buf.assign(&buf2);
	printf("\n\nsize: %d, capacity:%d\n", buf.size(), buf.capacity());
	for (int i=0; i<buf.size(); i++)
		printf("%x,", buf[i]);
}

int main(int argc, char* argv[])
{
	//test(); return 0; //just for debug

	// memory leak detection
	InitMemoryLeackReport( _CRTDBG_FILE_STDOUT );
	try
	{
		//find IsBarbaServer
		bool isBarbaServer = GetPrivateProfileInt(_T("General"), _T("ServerMode"), 0, BarbaApp::GetSettingsFile())!=0;

		//find command line parameter
		bool delayStart = false;
		for (int i=0; i<argc; i++)
		{
			if (_tcsicmp(argv[i], _T("/delaystart"))==0)
				delayStart = true;
		}

		//create App
		theApp = isBarbaServer ? (BarbaApp*)new BarbaServerApp(delayStart) : (BarbaApp*)new BarbaClientApp() ;
		theApp->Initialize();
		InitMemoryLeackReport(theApp->Comm.GetNotifyFileHandle());
		theApp->Start();
	}
	catch(BarbaException* er)
	{
		BarbaLog(_T("Error: %s"), er->ToString());
		delete er;
	}
	catch(LPCTSTR msg)
	{
		BarbaLog(_T("Error: %s"), msg);
	}
	catch(...)
	{
		BarbaLog(_T("Error: Unhandled Exception!"));
	}


	//dispose app
	if (theApp!=NULL)
		delete theApp;

	return 0;
}
