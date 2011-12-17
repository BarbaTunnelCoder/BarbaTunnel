#include "StdAfx.h"
#include "BarbaApp.h"

BarbaApp* theApp = NULL;
extern CNdisApi	api;

BarbaApp::BarbaApp(void)
{
	srand((UINT)time(0));
	_stprintf_s(_ConfigFile, _T("%s\\BarbaTunnel.ini"), GetModuleFolder());
	ZeroMemory ( &CurrentRequest, sizeof(ETH_REQUEST) );
	ZeroMemory ( &_PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	CurrentRequest.EthPacket.Buffer = &_PacketBuffer;

	_AdapterIndex = GetPrivateProfileInt(_T("General"), _T("AdapterIndex"), 0, GetConfigFile());
	_IsDebugMode = GetPrivateProfileInt(_T("General"), _T("DebugMode"), 0, GetConfigFile())!=0;
	BarbaSocket::InitializeLib();
}

BarbaApp::~BarbaApp(void)
{
	BarbaSocket::UninitializeLib();
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


bool BarbaApp::CheckTerminateCommands(PacketHelper* packet, bool send)
{
	if (!send || !packet->IsIp())
		return false;

	if (packet->ipHeader->ip_p!=1)
		return false;

	int nlen = packet->GetIpLen();
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

bool BarbaApp::SendPacketToMstcp(PacketHelper* packet)
{
	INTERMEDIATE_BUFFER intBuf = {0};
	intBuf.m_dwDeviceFlags = PACKET_FLAG_ON_RECEIVE;
	intBuf.m_Length = min(MAX_ETHER_FRAME, packet->GetPacketLen());
	memcpy_s(intBuf.m_IBuffer, MAX_ETHER_FRAME, packet->GetPacket(), intBuf.m_Length);

	ETH_REQUEST req;
	req.hAdapterHandle = theApp->CurrentRequest.hAdapterHandle;
	req.EthPacket.Buffer = &intBuf;
	return api.SendPacketToMstcp(&req)!=FALSE;
}

bool BarbaApp::SendPacketToAdapter(PacketHelper* packet)
{
	INTERMEDIATE_BUFFER intBuf = {0};
	intBuf.m_dwDeviceFlags = PACKET_FLAG_ON_SEND;
	intBuf.m_Length = min(MAX_ETHER_FRAME, packet->GetPacketLen());
	memcpy_s(intBuf.m_IBuffer, MAX_ETHER_FRAME, packet->GetPacket(), intBuf.m_Length);

	ETH_REQUEST req;
	req.hAdapterHandle = theApp->CurrentRequest.hAdapterHandle;
	req.EthPacket.Buffer = &intBuf;
	return api.SendPacketToAdapter(&req)!=FALSE;
}

