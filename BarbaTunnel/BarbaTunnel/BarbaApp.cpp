#include "StdAfx.h"
#include "BarbaApp.h"

BarbaApp* theApp = NULL;

BarbaApp::BarbaApp(void)
{
	srand(time(0));
	_stprintf_s(_ConfigFile, _T("%s\\BarbaTunnel.ini"), GetModuleFolder());
	ZeroMemory ( &CurrentRequest, sizeof(ETH_REQUEST) );
	ZeroMemory ( &_PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	CurrentRequest.EthPacket.Buffer = &_PacketBuffer;

	_AdapterIndex = GetPrivateProfileInt(_T("General"), _T("AdapterIndex"), 0, GetConfigFile());
	_IsDebugMode = GetPrivateProfileInt(_T("General"), _T("DebugMode"), 0, GetConfigFile())!=0;
}

LPCTSTR BarbaApp::GetConfigFile()
{
	static TCHAR configFile[MAX_PATH] = {0};
	if (configFile[0]==0)
	{
		_stprintf_s(configFile, _T("%s\\BarbaTunnel.ini"), GetModuleFolder());
	}
	return configFile;
}

LPCTSTR BarbaApp::GetModuleFile()
{
	static TCHAR moduleFile[MAX_PATH] = {0};
	if (moduleFile[0]==0)
	{
		::GetModuleFileName(NULL, moduleFile, MAX_PATH);
	}
	return moduleFile;
}

LPCTSTR BarbaApp::GetModuleFolder()
{
	static TCHAR moduleFolder[MAX_PATH] = {0};
	if (moduleFolder[0]==0)
	{
		BarbaUtils::GetModuleFolder(moduleFolder);
	}
	return moduleFolder;
}

BarbaApp::~BarbaApp(void)
{
}

bool BarbaApp::CheckTerminateCommands(INTERMEDIATE_BUFFER* packetBuffer)
{
	bool send = packetBuffer->m_dwDeviceFlags == PACKET_FLAG_ON_SEND;
	PacketHelper packet(packetBuffer);

	if (send || !packet.IsIp())
		return false;

	if (packet.ipHeader->ip_p!=1)
		return false;

	int nlen = packet.GetIpLen();
	int code = nlen - 28;
	if (code==1350)
			return true;

	return false;
}

void BarbaApp::Dispose()
{
	Comm.Dispose();
}

void BarbaApp::Initialize()
{
	Comm.Initialize();
}

