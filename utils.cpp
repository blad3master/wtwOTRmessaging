#include "stdafx.h"
#include "utils.h"
#include <string>
#include <codecvt> // std::codecvt
#include <cassert>
#include <strsafe.h>
#include "Logger.h"


utf8String::utf8String(const char * src)
{
	const size_t src_len = strnlen_s(src, _max_len);
	if (src_len >= _max_len) {
		LOG_CRITICAL(L"%s() string is too long - truncating to %llu!", __FUNCTIONW__, (unsigned long long)_max_len);
	}

	_data_len = src_len + 1;
	_data_ptr = new (std::nothrow) char[_data_len];
	if (nullptr != _data_ptr)
	{
		std::memcpy(_data_ptr, src, _data_len*sizeof(char));
	}

	/*

	// BELOW is some code for htmlEntitiesStripped() function

			const char * s = src;
			const char * end = src + src_len;
			char * d = _data_ptr;
			while (s < end)
			{
				if (*s != '&') {
					*d++ = *s++;
				}
				else
				{
					if (((s + 4) < end) && (0 == strncmp("&amp;", s, 5))) {
						s += 5;
						*d++ = '&';
					}
					else if (((s + 5) < end) && (0 == strncmp("&quot;", s, 6))) {
						s += 6;
						*d++ = '"';
					}
				}
			}
			*d = 0;
	*/
}

utf8String::utf8String(const utf8String &other) : utfXStringBase(other)
{
}

utf8String::utf8String(utf8String &&other) : utfXStringBase(std::move(other))
{
}

utf8String& utf8String::operator = (const char *src)
{
	*this = utf8String(src);

	return *this;
}

utf8String& utf8String::operator =(const utf8String &other)
{
	utfXStringBase::operator=(other);
	return *this;
}

utf8String& utf8String::operator =(utf8String &&other)
{
	utfXStringBase::operator=(std::move(other));
	return *this;
}


utf16String::utf16String(const wchar_t * src)
{
	const size_t src_len = wcsnlen_s(src, _max_len);
	if (src_len >= _max_len) {
		LOG_CRITICAL(L"%s() string is too long - truncating to %llu!", __FUNCTIONW__, (unsigned long long)_max_len);
	}

	_data_len = src_len + 1;
	_data_ptr = new (std::nothrow) wchar_t[_data_len];
	if (nullptr != _data_ptr)
	{
		std::memcpy(_data_ptr, src, _data_len*sizeof(wchar_t));
	}
}

utf16String::utf16String(const utf16String &other) : utfXStringBase(other)
{
}

utf16String::utf16String(utf16String &&other) : utfXStringBase(std::move(other))
{
}


utf16String& utf16String::operator = (const wchar_t *src)
{
	*this = utf16String(src);

	return *this;
}

utf16String& utf16String::operator =(const utf16String &other)
{
	utfXStringBase::operator=(other);
	return *this;
}

utf16String& utf16String::operator =(utf16String &&other)
{
	utfXStringBase::operator=(std::move(other));
	return *this;
}



utf16String utf16String::htmlTagsStripped() const
{
	utf16String u16_stripped;
	const wchar_t * src = _data_ptr;
	const wchar_t * end = src + _data_len;
	wchar_t       * dst = new (std::nothrow) wchar_t[_data_len];
	
	if ((nullptr != _data_ptr) && (nullptr != dst))
	{
		delete[] u16_stripped._data_ptr; // not necessary, but let it be here
		u16_stripped._data_ptr = dst;
		u16_stripped._data_len = _data_len;

		int b_l = 0;
		int b_r = 0;
		int font_l = 0;
		int font_r = 0;

		while (src < end)
		{
			if (*src != L'<') {
				*dst++ = *src++;
			}
			else
			{
				if (((src + 2) < end) && (0 == wcsncmp(L"<b>", src, 3))) {
					++b_l;
					src += 3;
				}
				else if (((src + 3) < end) && (0 == wcsncmp(L"</b>", src, 4))) {
					++b_r;
					src += 4;
				}
				else if (((src + 4) < end) && (0 == wcsncmp(L"<font", src, 5))) {
					++font_l;
					src += 5;
					for (; (src < end) && (*src != L'>'); ++src);
				}
				else if (((src + 6) < end) && (0 == wcsncmp(L"</font>", src, 7))) {
					++font_r;
					src += 7;
				}
				else {
					*dst++ = *src++;
				}
			}
		}
		if (static_cast<size_t>(dst - u16_stripped._data_ptr) < u16_stripped._data_len) {
			*dst = 0;
		}
		else {
			u16_stripped._data_ptr[u16_stripped._data_len - 1] = 0; //should be already copied from src but left for clarity
		}

		if (b_l != b_r) {
			LOG_ERROR(L"%s() possible wrong HTML formmating <b></b>? (before='%s', stripped='%s')", __FUNCTIONW__,
				_data_ptr, u16_stripped._data_ptr);
		}
		if (b_l != b_r) {
			LOG_ERROR(L"%s() possible wrong HTML formmating <font></font>? (before='%s', stripped='%s')", __FUNCTIONW__,
				_data_ptr, u16_stripped._data_ptr);
		}
	}

	return std::move(u16_stripped);
}

utf16String utf16String::htmlEntitiesStripped() const
{
	utf16String u16_stripped;
	const wchar_t * src = _data_ptr;
	const wchar_t * end = src + _data_len;
	wchar_t       * dst = new (std::nothrow) wchar_t[_data_len];

	if ((nullptr != _data_ptr) && (nullptr != dst))
	{
		delete[] u16_stripped._data_ptr; // not necessary, but let it be here
		u16_stripped._data_ptr = dst;
		u16_stripped._data_len = _data_len;

		while (src < end)
		{
			if (*src != L'&') {
				*dst++ = *src++;
			}
			else
			{
				if (((src + 4) < end) && (0 == wcsncmp(L"&amp;", src, 5))) {
					src += 5;
					*dst++ = L'&';
				}
				else if (((src + 5) < end) && (0 == wcsncmp(L"&quot;", src, 6))) {
					src += 6;
					*dst++ = L'"';
				}
				else if (((src + 3) < end) && (0 == wcsncmp(L"&lt;", src, 4))) {
					src += 4;
					*dst++ = L'<';
				}
				else if (((src + 3) < end) && (0 == wcsncmp(L"&gt;", src, 4))) {
					src += 4;
					*dst++ = L'>';
				}
				else {
					*dst++ = *src++;
				}
			}
		}
		if (static_cast<size_t>(dst - u16_stripped._data_ptr) < u16_stripped._data_len) {
			*dst = 0;
		}
		else {
			u16_stripped._data_ptr[u16_stripped._data_len - 1] = 0; //should be already copied from src but left for clarity
		}
	}

	return std::move(u16_stripped);
}


/*
void fingerprintToWString(const unsigned char fp[20], wchar_t *dst)
{
int dst_index = 0;

for (int i = 0; i < 20; ++i)
{
wsprintf(&dst[dst_index], L"%02X", fp[i]);
dst_index += 2;

if (3 == i || 7 == i || 11 == i || 15 == i)
{
dst[dst_index++] = L' ';
}
}

dst[dst_index] == ' ';
}
*/


static const int max_str_len = 4096;

#ifdef _WTWOTRMESSAGING_USE_DYNAMIC_STRINGS

utf8String utf16Toutf8(const wchar_t *src)
{
	assert(src);
	assert(wcsnlen_s(src, max_str_len) < max_str_len);

	utf8String u8;

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

	try {
		// make the copy of conversion to u8
		u8 = convert.to_bytes(reinterpret_cast<const char16_t*>(src)).data();
	}
	catch (std::range_error &e) {
		u8 = "";
		LOG_CRITICAL(L"%s() range_error exception: %s", __FUNCTIONW__, utf8Toutf16(e.what()).operator LPCWSTR());
		assert(0);
	}

	return  std::move(u8);
}

utf16String utf8Toutf16(const char *src)
{
	assert(src);
	assert(strnlen_s(src, max_str_len) < max_str_len);

	utf16String u16;

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

	try {
		// make the copy of conversion to u16
		u16 = reinterpret_cast<const wchar_t*>(convert.from_bytes(src).data());
	}
	catch (std::range_error &e) {
		u16 = L"";
		LOG_CRITICAL(L"%s() range_error exception: %s", __FUNCTIONW__, utf8Toutf16(e.what()).operator LPCWSTR());
		assert(0);
	}

	return std::move(u16);
}



utf16String utf8Toutf16_safe(const char *src, size_t length)
{
	assert(src);
	assert(length < max_str_len);

	char buf[max_str_len];
	utf16String u16;
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

	for (size_t i = 0; i < length; ++i)
	{
		char c = src[i];
		buf[i] = ((c>0) && isprint(c)) ? c : '.';
	}
	buf[length] = 0;

	try {
		// make the copy of conversion to u16
		u16 = reinterpret_cast<const wchar_t*>(convert.from_bytes(buf).data());
	}
	catch (std::range_error &e) {
		u16 = L"";
		LOG_CRITICAL(L"%s() range_error exception: %s", __FUNCTIONW__, utf8Toutf16(e.what()).operator LPCWSTR());
		assert(0);
	}

	return u16;
}

#else

static const int utf8_conv_array_size = 32;
static int utf8_conv_array_index = 0;
static std::string  utf8_conv_array[utf8_conv_array_size];

static const int utf16_conv_array_size = utf8_conv_array_size;
static int utf16_conv_array_index = 0;
static std::wstring  utf16_conv_array[utf16_conv_array_size];



const char * utf16Toutf8(const wchar_t *src)
{
	assert(src);
	assert(wcsnlen_s(src, max_str_len) < max_str_len);

	if (++utf8_conv_array_index >= utf8_conv_array_size) {
		utf8_conv_array_index = 0;
	}

	std::string &u8 = utf8_conv_array[utf8_conv_array_index];

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

	try {
		// make the copy of conversion to u8
		u8 = convert.to_bytes(reinterpret_cast<const char16_t*>(src));
	}
	catch (std::range_error &e) {
		u8 = "";
		LOG_CRITICAL(L"%s() range_error exception", __FUNCTIONW__);
		assert(0);
	}

	return u8.data();
}

const wchar_t * utf8Toutf16(const char *src)
{
	assert(src);
	assert(strnlen_s(src, max_str_len) < max_str_len);

	if (++utf16_conv_array_index >= utf16_conv_array_size) {
		utf16_conv_array_index = 0;
	}

	std::wstring &u16 = utf16_conv_array[utf16_conv_array_index];

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

	try {
		// make the copy of conversion to u16
		u16 = reinterpret_cast<const wchar_t*>(convert.from_bytes(src).data());
	}
	catch (std::range_error &e) {
		u16 = L"";
		LOG_CRITICAL(L"%s() range_error exception", __FUNCTIONW__);
		assert(0);
	}

	return u16.data();
}



const wchar_t * utf8Toutf16_safe(const char *src, size_t length)
{
	assert(src);
	assert(length < max_str_len);

	char buf[max_str_len];

	if (++utf16_conv_array_index >= utf16_conv_array_size) {
		utf16_conv_array_index = 0;
	}

	std::wstring &u16 = utf16_conv_array[utf16_conv_array_index];
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

	for (size_t i = 0; i < length; ++i)
	{
		char c = src[i];
		buf[i] = ((c>0) && isprint(c)) ? c : '.';
	}
	buf[length] = 0;

	try {
		u16 = L""; // if from_bytes() will throw, function will not return value of last string
		// make the copy of conversion to u16
		u16 = reinterpret_cast<const wchar_t*>(convert.from_bytes(buf).data());
	}
	catch (std::range_error &e) {
		LOG_CRITICAL(L"%s() range_error exception", __FUNCTIONW__);
		assert(0);
	}

	return u16.data();
}

#endif // _WTWOTRMESSAGING_USE_DYNAMIC_STRINGS


std::wstring makeKeyFromPeer(const wchar_t *id, const wchar_t *netClass, int netId)
{
	wchar_t buf[512];
	StringCbPrintfW(buf, sizeof(buf), L"%s/%s/%d", id, netClass, netId);
	return std::wstring(buf);
}


std::wstring makeKeyFromPeer(const wtwContactDef *contact)
{
	return makeKeyFromPeer(contact->id, contact->netClass, contact->netId);
}