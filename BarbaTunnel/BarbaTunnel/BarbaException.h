#pragma once
class BarbaException
{
public:
	explicit BarbaException(void)
	{
		Description[0] = 0;
	}
	
	explicit BarbaException(LPCTSTR description, ...)
	{
		va_list argp;
		va_start(argp, description);
		_vstprintf_s(this->Description, description, argp);
		va_end(argp);
	}
	virtual ~BarbaException(void){}
	LPCTSTR ToString() {return this->Description;}

protected:
	TCHAR Description[2000];
};

