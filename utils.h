#pragma once

#include <string>

//void fingerprintToWString(const unsigned char fp[20], wchar_t *dst);

const char * utf16Toutf8(const wchar_t *src);

const wchar_t * utf8Toutf16(const char *src);

