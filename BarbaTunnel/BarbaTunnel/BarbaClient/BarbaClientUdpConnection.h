#pragma once
#include "BarbaClientConnection.h"

class BarbaClientUdpConnection :
	public BarbaClientConnection
{
public:
	explicit BarbaClientUdpConnection(LPCTSTR connectionName, BarbaKey* key);
	virtual ~BarbaClientUdpConnection(void);
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packetBuffer);
	virtual BarbaModeEnum GetMode();

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacketBuffer);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer);

};

