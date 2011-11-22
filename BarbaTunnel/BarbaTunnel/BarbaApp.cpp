#include "StdAfx.h"
#include "BarbaApp.h"


BarbaApp::BarbaApp(void)
{
	BarbaUtils::GetModuleFolder(ModuleFolder);
	_stprintf_s(ConfigFile, _T("%s\\config.ini"), ModuleFolder);
	ZeroMemory ( &CurrentRequest, sizeof(ETH_REQUEST) );
	ZeroMemory ( &PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	CurrentRequest.EthPacket.Buffer = &PacketBuffer;
	IsDebugMode = false;

	AdapterIndex = GetPrivateProfileInt(_T("General"), _T("AdapterIndex"), 0, GetConfigFile());
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