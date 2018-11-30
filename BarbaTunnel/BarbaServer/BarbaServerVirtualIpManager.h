#pragma once
#include "General.h"

class BarbaServerVirtualIpManager
{
public:
	void Initialize(IpRange* virtualIpRange)
	{
		memcpy_s(&VirtualIpRange, sizeof(IpRange), virtualIpRange, sizeof(IpRange));
	}

	BarbaServerVirtualIpManager()
	{
		memset(VirtualIps, 0, sizeof(VirtualIps));
		memset(&VirtualIpRange, 0, sizeof(VirtualIpRange));
	}
	
	DWORD GetNewIp()
	{
		int maxIp = ntohl(VirtualIpRange.EndIp) - (VirtualIpRange.StartIp) + 1;
		for (int i=0; i<_countof(VirtualIps) && i<maxIp; i++)
		{
			if ( !VirtualIps[i] )
			{
				VirtualIps[i] = true;
				return htonl( ntohl(VirtualIpRange.StartIp) + i );
			}
		}
		return 0;
	}

	void ReleaseIp(DWORD Ip)
	{
		int index = ntohl(Ip) - ntohl(VirtualIpRange.StartIp);
		if (index>0 && index<BARBA_ServerMaxVirtualIps)
			VirtualIps[index] = false;
	}

private:
	bool VirtualIps[BARBA_ServerMaxVirtualIps];
	IpRange VirtualIpRange;
};

