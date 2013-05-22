#include "stdafx.h"
#include "BarbaPortRange.h"
#include "BarbaUtils.h"


BarbaPortRange::BarbaPortRange(void)
{
}


BarbaPortRange::~BarbaPortRange(void)
{
}

size_t BarbaPortRange::GetPortsCount()
{
	size_t ret = 0;
	for (size_t i=0; i<Items.size(); i++)
	{
		int count = Items[i].EndPort - Items[i].StartPort + 1;
		ret += count;
	}
	return ret;
}

void BarbaPortRange::GetAllPorts(BarbaArray<u_short>* ports)
{
	ports->reserve(GetPortsCount());

	for (size_t i=0; i<Items.size(); i++)
	{
		for (size_t j=Items[i].StartPort; j<=Items[i].EndPort; j++)
		{
			ports->append((u_short)j);
		}
	}
}

bool BarbaPortRange::IsPortInRange(u_short port)
{
	for (size_t i=0; i<Items.size(); i++)
	{
		if (port>=Items[i].StartPort && port<=Items[i].EndPort)
			return true;
	}
	return false;
}

u_short BarbaPortRange::GetRandomPort()
{
	u_int newPortIndex = BarbaUtils::GetRandom(0, (u_int)GetPortsCount()-1);
	for (size_t i=0; i<Items.size(); i++)
	{
		u_int count = Items[i].EndPort - Items[i].StartPort + 1;
		if (newPortIndex<count)
			return (u_short)(Items[i].StartPort + newPortIndex);
		newPortIndex -= count;
	}

	throw new BarbaException(_T("Could not find any port!"));
}

void BarbaPortRange::Parse(LPCTSTR value)
{
	BarbaBuffer sbuffer((BYTE*)value, _tcslen(value)*2+2);
	std::tstring buffer = value;
	if (buffer.empty())
		return;

	TCHAR* currentPos = NULL;
	LPCTSTR token = _tcstok_s(&buffer.front(), _T(","), &currentPos);
		
	while (token!=NULL)
	{
		PortRangeItem portRangeItem;
		if (ParsePortRangeItem(token, &portRangeItem.StartPort, &portRangeItem.EndPort))
			Items.push_back(portRangeItem);
		token = _tcstok_s(NULL, _T(","), &currentPos);
	}
}

bool BarbaPortRange::ParsePortRangeItem(LPCTSTR value, u_short* startPort, u_short* endPort)
{
	//VirtualIpRange
	TCHAR* dash = _tcschr((TCHAR*)value, '-');
	
	TCHAR ipBuffer[100];
	_tcsncpy_s(ipBuffer, _countof(ipBuffer), value, dash!=NULL ? dash-value : _tcslen(value));
	*startPort = (u_short)_tcstoul(ipBuffer, NULL, 0);
	*endPort = *startPort; //default
	if (dash!=NULL)
	{
		_tcscpy_s(ipBuffer, _countof(ipBuffer), dash+1);
		*endPort = (u_short)_tcstoul(ipBuffer, NULL, 0);
	}

	return *startPort!=0 && (*endPort-*startPort)>=0;
}
