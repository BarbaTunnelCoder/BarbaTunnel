#pragma once

class Base64
{
public:
	static std::tstring encode(std::vector<BYTE>* buffer);
	static std::tstring encode(BYTE* bytes_to_encode, unsigned int in_len);
	static void decode(std::string const& encoded_string, std::vector<BYTE>& ret);

private:
	static inline bool is_base64(unsigned char c) 
	{
		return (isalnum(c) || (c == '+') || (c == '/'));
	}
};

