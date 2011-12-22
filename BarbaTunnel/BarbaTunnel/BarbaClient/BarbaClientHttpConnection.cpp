#include "StdAfx.h"
#include "BarbaClientHttpConnection.h"
#include "BarbaClientApp.h"
#include "StringUtil.h"


BarbaClientHttpConnection::BarbaClientHttpConnection(BarbaClientConfig* config, BarbaClientConfigItem* configItem, u_short tunnelPort)
	: BarbaClientConnection(config, configItem)
	, Courier()
{
	this->SessionId = BarbaUtils::GetRandom(100000, UINT_MAX-1);
	CHAR sessionBuffer[BARBA_MaxKeyName+20];
	sprintf_s(sessionBuffer, "%s=%x", config->SessionKeyName, this->SessionId);
	std::string getTemplate = theClientApp->FakeHttpGetTemplate;
	StringUtil::ReplaceAll(getTemplate, "{session}", sessionBuffer);
	std::string postTemplate = theClientApp->FakeHttpPostTemplate;
	StringUtil::ReplaceAll(postTemplate, "{session}", sessionBuffer);

	this->TunnelPort = tunnelPort;
	this->Courier = new BarbaClientHttpCourier(configItem->MaxUserConnections, config->ServerIp, tunnelPort, 
		getTemplate.data(), postTemplate.data(), this);
}

BarbaClientHttpConnection::~BarbaClientHttpConnection(void)
{
	theApp->AddThread(this->Courier->Delete());
}

bool BarbaClientHttpConnection::ShouldProcessPacket(PacketHelper* packet)
{
	//just process outgoing packets
	return packet->GetDesIp()==GetServerIp() && ConfigItem->ShouldGrabPacket(packet);
}

bool BarbaClientHttpConnection::ProcessPacket(PacketHelper* packet, bool send)
{
	if (send)
	{
		this->SetWorkingState(packet->GetIpLen(), true);
		this->Courier->SendPacket(packet);
	}
	else
	{
		this->SendPacketToMstcp(packet);
	}
	return true;
}
