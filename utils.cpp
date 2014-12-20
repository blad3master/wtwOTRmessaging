#include "stdafx.h"
#include "utils.h"
#include <string>
#include <codecvt> // std::codecvt


static const int utf8_conv_array_size = 16;
static int utf8_conv_array_index = 0;
static std::string  utf8_conv_array[utf8_conv_array_size];

static const int utf16_conv_array_size = utf8_conv_array_size;
static int utf16_conv_array_index = 0;
static std::wstring  utf16_conv_array[utf8_conv_array_size];

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


const char * utf16Toutf8(const wchar_t *src)
{
	if (++utf8_conv_array_index >= utf8_conv_array_size) {
		utf8_conv_array_index = 0;
	}

	std::string &u8 = utf8_conv_array[utf8_conv_array_index];

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

	// make the copy of conversion to u8
	u8 = convert.to_bytes(reinterpret_cast<const char16_t*>(src));

	return u8.data();
}

const wchar_t * utf8Toutf16(const char *src)
{
	if (++utf16_conv_array_index >= utf16_conv_array_size) {
		utf16_conv_array_index = 0;
	}

	std::wstring &u16 = utf16_conv_array[utf16_conv_array_index];

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

	// make the copy of conversion to u16
	u16 = reinterpret_cast<const wchar_t*>(convert.from_bytes(src).data());

	return u16.data();
}
