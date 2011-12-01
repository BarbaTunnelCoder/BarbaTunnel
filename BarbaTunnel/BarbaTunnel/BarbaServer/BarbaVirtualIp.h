#pragma once

class BarbaVirtualIpManager
{
public:
	BarbaVirtualIpManager(IpRange* virtualIpRange)
	{
		memset(VirtualIps, 0, sizeof(VirtualIps));
		VirtualIpRange = virtualIpRange;
	}
	
	DWORD GetNewIp()
	{
		int maxIp = VirtualIpRange->EndIp - VirtualIpRange->StartIp + 1;
		for (int i=0; i<_countof(VirtualIps) && i<maxIp; i++)
		{
			if ( !VirtualIps[i] )
			{
				VirtualIps[i] = true;
				return htonl( ntohl(VirtualIpRange->StartIp) + i );
			}
		}
		return 0;
	}

	void ReleaseIp(DWORD Ip)
	{
		int index = ntohl(Ip) - ntohl(VirtualIpRange->StartIp);
		if (index>0 && index<BARBA_MAX_VIRTUALIP)
			VirtualIps[index] = false;
	}

private:
	bool VirtualIps[BARBA_MAX_VIRTUALIP];
	IpRange* VirtualIpRange;
};

