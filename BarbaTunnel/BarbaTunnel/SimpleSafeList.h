#pragma once
#include <list>
#include "SimpleCriticalSection.h"

template <class _Ty> 
class SimpleSafeList
{
private:
	std::list<_Ty> _list;
	SimpleCriticalSection _cs;
	_Ty* _LockedBuffer;

public:
	class AutoLockBuffer
	{
	public:
		explicit AutoLockBuffer(SimpleSafeList<_Ty>* list)
		{
			this->_List = list;
			this->_Buffer = list->LockBuffer();
		}
		~AutoLockBuffer()
		{
			Unlock();
		}

		_Ty* GetBuffer()
		{
			return _Buffer;
		}

		void Unlock()
		{
			if (_Buffer!=NULL)
			{
				this->_List->UnlockBuffer();
				_Buffer = NULL;
			}
		}
	private:
		SimpleSafeList<_Ty>* _List;
		_Ty* _Buffer;
	};

public:
	SimpleSafeList() {_LockedBuffer=NULL;}
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

	//pointer to array
	_Ty* LockBuffer()
	{
		_cs.Enter();
		if (_list.size()==0)
			return NULL;

		if (_LockedBuffer==NULL)
		{
			_LockedBuffer = new _Ty[_list.size()];
			memset(_LockedBuffer, 0, _list.size());
			size_t size = _list.size();
			GetAll(_LockedBuffer, &size);
		}
		return _LockedBuffer;
	}

	//pointer to array
	void UnlockBuffer()
	{
		if (_LockedBuffer!=NULL)
		{
			delete _LockedBuffer;
			_LockedBuffer = NULL;
			_cs.Leave();
		}
	}

	SimpleCriticalSection* GetCriticalSection() 
	{
		return &_cs;
	}

private:
	//not thread safe
	void GetAll(_Ty buffer[], size_t* count)
	{
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
};
