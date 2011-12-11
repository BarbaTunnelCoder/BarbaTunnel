#pragma once
#include <list>
#include "SimpleCriticalSection.h"

template <class _Ty> 
class SimpleSafeList
{
private:
	std::list<_Ty> _list;
	SimpleCriticalSection _cs;

public:
	bool IsEmpty() {SimpleLock lock(&_cs); return _list.empty();}
	void AddHead(_Ty _Val) {SimpleLock lock(&_cs); _list.push_front(_Val);}
	void AddTail(_Ty _Val) {SimpleLock lock(&_cs); _list.push_back(_Val);}
	_Ty RemoveTail()
	{
		SimpleLock lock(&_cs); 
		if (_list.empty())
			return NULL;
		_Ty ret = _list.back(); 
		_list.pop_back();
		return ret;
	}

	_Ty RemoveHead()
	{
		SimpleLock lock(&_cs); 
		if (_list.empty())
			return NULL;
		_Ty ret = _list.front(); 
		_list.pop_front();
		return ret;
	}

	void Remove(_Ty _Val)
	{
		SimpleLock lock(&_cs); 
		_list.remove(_Val);
	}

	size_t GetCount()
	{
		SimpleLock lock(&_cs); 
		return _list.size();
	}

	void GetAll(_Ty buffer[], size_t* count)
	{
		//SimpleLock lock(&_cs); 
		if (_list.empty())
		{
			*count = 0;
			return;
		}

		size_t i = 0;
		std::list<_Ty>::iterator iter = _list.begin();
		for ( iter = _list.begin(); i<*count && iter != _list.end(); iter++ )
		{
			buffer[i++] = *iter;
		}

		*count = i;
	}

	
	SimpleCriticalSection* GetCriticalSection() 
	{
		return &_cs;
	}
};
