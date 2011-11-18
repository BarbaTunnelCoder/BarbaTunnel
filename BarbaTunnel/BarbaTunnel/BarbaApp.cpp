#include "StdAfx.h"
#include "BarbaApp.h"


BarbaApp::BarbaApp(void)
{
	ZeroMemory ( &CurrentRequest, sizeof(ETH_REQUEST) );
	ZeroMemory ( &PacketBuffer, sizeof(INTERMEDIATE_BUFFER) );
	CurrentRequest.EthPacket.Buffer = &PacketBuffer;
	IsDebugMode = false;
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