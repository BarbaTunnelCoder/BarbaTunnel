#pragma once

class StringUtils
{
public:
	static u_int ReplaceAll(std::string& str, const std::string& from, const std::string& to) 
	{
		u_int ret = 0;
		size_t start_pos = 0;
		while((start_pos = str.find(from, start_pos)) != std::string::npos) 
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
			ret++;
		}
		return ret;
	}

	static void Trim(std::tstring& str, TCHAR chr=' ')
	{
		std::tstring::size_type pos = str.find_last_not_of(chr);
		if(pos != std::tstring::npos) 
		{
			str.erase(pos + 1);
			pos = str.find_first_not_of(chr);
			if(pos != std::tstring::npos) 
				str.erase(0, pos);
		}
		else
		{
			str.erase(str.begin(), str.end());
		}
	}

	static void Tokenize(LPCTSTR text, LPCTSTR delimiter, std::vector<std::tstring>* tokens)
	{
		std::string s;
		
		size_t bufferLen = _tcslen(text)+2;
		TCHAR* buffer = new TCHAR[bufferLen];
		_tcscpy_s(buffer, bufferLen, text);
		TCHAR* currentPos = NULL;
		LPCTSTR token = _tcstok_s(buffer, delimiter, &currentPos);
		
		while (token!=NULL)
		{
			std::tstring item = token;
			Trim(item);
			if (item.size()>0)
				tokens->push_back(item);
			token = _tcstok_s(NULL, delimiter, &currentPos);
		}

		delete buffer;
	}

	static void MakeLower( std::tstring& str )
	{
		for (int i=0; i<(int)str.size(); i++)
			str[i] = (TCHAR)_totlower(str[i]);
	}
};

