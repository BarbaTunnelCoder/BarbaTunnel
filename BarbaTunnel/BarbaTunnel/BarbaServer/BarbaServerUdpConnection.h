#pragma once
#include "BarbaServerConnection.h"

class BarbaServerUdpConnection : public BarbaServerConnection
{
public:
	explicit BarbaServerUdpConnection(LPCTSTR connectionName, BarbaKey* key);
	virtual ~BarbaServerUdpConnection(void);
	virtual bool ProcessPacket(INTERMEDIATE_BUFFER* packet);
	virtual BarbaModeEnum GetMode();

private:
	bool CreateUdpBarbaPacket(PacketHelper* packet, BYTE* barbaPacket);
	bool ExtractUdpBarbaPacket(PacketHelper* barbaPacket, BYTE* orgPacketBuffer);

};

