#include "stdafx.h"
#include "BarbaClient\BarbaClientApp.h"
#include "BarbaServer\BarbaServerApp.h"
#include "WinpkFilterDriver.h"

BarbaFilterDriver* CreateFilterDriverByName(LPCTSTR name)
{
	if (_tcsicmp(name, _T("WinpkFilter"))==0)
		return new WinpkFilterDriver();

	throw new BarbaException(_T("Invalid FilterDriver: %s"), name);
}

void InitMemoryLeackReport()
{
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );
}

int test()
{
	std::tstring folder = BarbaUtils::GetFileFolderFromUrl("d:\\mmm\\sss\\bb.txt\\");
	std::tstring folderName = BarbaUtils::GetFileNameFromUrl(folder.data());
	printf("%s\n%s\n", folder.data(), folderName.data());
	return 0;
}

int main(int argc, char* argv[])
{
	//test(); return 0; //just for debug

	// memory leak detection
	InitMemoryLeackReport();
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
