#pragma once

#include <string>
#include "Logger.h"


template <typename T>
struct utfXStringBase
{
protected:

	utfXStringBase() {};

	utfXStringBase(const utfXStringBase &other)
	{
		operator=(other);
	}

	utfXStringBase(utfXStringBase &&other)
	{
		operator=(std::move(other));
	}

	virtual  ~utfXStringBase()
	{
		if (nullptr != _data_ptr)
		{
			delete[] _data_ptr;
			_data_ptr = nullptr;
		}
	}

	utfXStringBase& operator = (const utfXStringBase& other)
	{
		if (this != &other)
		{
			if (nullptr != _data_ptr) {
				delete[] _data_ptr;
			}
			_data_len = other._data_len;
			_data_ptr = new (std::nothrow) T[_data_len];
			if (nullptr != _data_ptr) {
				std::memcpy(_data_ptr, other._data_ptr, _data_len*sizeof(T));
			}
		}

		return *this;
	}

	utfXStringBase& operator = (utfXStringBase &&other)
	{
		if (nullptr != _data_ptr) {
			delete[] _data_ptr;
		}
		_data_ptr = other._data_ptr;
		_data_len = other._data_len;
		other._data_ptr = nullptr;
		other._data_len = 0;

		return *this;
	}

	static const size_t _max_len = 4096;

	T * _data_ptr = nullptr;
	size_t _data_len = 0; // number of elements T
};

struct utf8String : private utfXStringBase<char>
{
	explicit utf8String() {};

	explicit utf8String(const char * src);

	utf8String(const utf8String &other);

	utf8String(utf8String &&other);

	utf8String& operator =(const char *src);

	utf8String& operator =(const utf8String &other);

	utf8String& operator =(utf8String &&other);
	
	inline operator const char*() const // utf8
	{
		return _data_ptr;
	}
};


struct utf16String : private utfXStringBase<wchar_t>
{
	explicit utf16String() {};

	explicit utf16String(const wchar_t * src);

	utf16String(const utf16String &other);

	utf16String(utf16String &&other);

	utf16String& operator =(const wchar_t *src);

	utf16String& operator =(const utf16String &other);

	utf16String& operator =(utf16String &&other);

	inline operator const wchar_t*() const // utf-16
	{
		return _data_ptr;
	}

	// return string with HTML tags removed
	utf16String htmlTagsStripped() const;

	// return string with HTML entities replaced
	utf16String htmlEntitiesStripped() const;
};



//void fingerprintToWString(const unsigned char fp[20], wchar_t *dst);


#define _WTWOTRMESSAGING_USE_DYNAMIC_STRINGS

#ifdef _WTWOTRMESSAGING_USE_DYNAMIC_STRINGS

	utf8String utf16Toutf8(const wchar_t *src);

	utf16String utf8Toutf16(const char *src);

	// this method might be used on strings whose length is know but content
	// may contain unprintable characters (including null-character '\0')
	// those "wrong" characters will be replaced by '.'
	utf16String utf8Toutf16_safe(const char *src, size_t length);

#else

	const char * utf16Toutf8(const wchar_t *src);

	const wchar_t * utf8Toutf16(const char *src);

	// this method might be used on strings whose length is know but content
	// may contain unprintable characters (including null-character '\0')
	// those "wrong" characters will be replaced by '.'
	const wchar_t * utf8Toutf16_safe(const char *src, size_t length);

#endif // _WTWOTRMESSAGING_USE_DYNAMIC_STRINGS

std::wstring makeKeyFromPeer(const wchar_t *id, const wchar_t *netClass, int netId);

std::wstring makeKeyFromPeer(const wtwContactDef *contact);
