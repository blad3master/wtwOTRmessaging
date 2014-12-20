#include "StdAfx.h"
#include "Logger.h"
#include "utils.h"

static const wchar_t LOG_PREFIX[] = L"OTR";

static wchar_t log_string_tmp_buffer[4 * 1024];

unsigned int Logger::loggingFlags(0xFFFFFFFF);


Logger::Logger(void)
{
}


Logger::~Logger(void)
{
}


void Logger::critical(const wchar_t *msg) {
	if (LOG_CRITICAL & loggingFlags) {
		__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_CRITICAL);
	}
}


void Logger::error(const wchar_t *msg) {
	if (LOG_ERROR & loggingFlags) {
		__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_ERROR);
	}
}


void Logger::warn(const wchar_t *msg) {
	if (LOG_WARN & loggingFlags) {
		__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_ERROR);
	}
}


void Logger::info(const wchar_t *msg) {
	if (LOG_INFO & loggingFlags) {
		__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_INFO);
	}
}


void Logger::debug(const wchar_t *msg) {
	if (LOG_DEBUG & loggingFlags) {
		__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_NORMAL);
		//__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_DEBUG);
	}
}


/*
const char * wcToCharString(const wchar_t *wcString)
{
#if 0
	static char buffer[CONVERT_BUF_NUM][CONVERT_BUF_LEN];
	static int bufIndex = 0;

	if ((++bufIndex) >= CONVERT_BUF_NUM)
		bufIndex = 0;

	if (0 == wcstombs_s(0, buffer[bufIndex], CONVERT_BUF_LEN, wcString, CONVERT_BUF_LEN))
		return buffer[bufIndex];

	return NULL;
#else
	return utf16Toutf8(wcString).data();
#endif
}


const wchar_t *cToWideCharString(const char* cString)
{
#if 0
	static wchar_t buffer[CONVERT_BUF_NUM][CONVERT_BUF_LEN];
	static int bufIndex = 0;

	if ((++bufIndex) >= CONVERT_BUF_NUM)
		bufIndex = 0;

	if (0 == mbstowcs_s(0, buffer[bufIndex], CONVERT_BUF_LEN, cString, CONVERT_BUF_LEN))
		return buffer[bufIndex];

	return NULL;
#else
	return utf8Toutf16(cString).data();
#endif
}
*/


extern "C" {

void __LOG_CRITICAL(const wchar_t *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vswprintf_s<sizeof(log_string_tmp_buffer) / sizeof(wchar_t)>(log_string_tmp_buffer, fmt, ap);
	va_end(ap);

	Logger::critical(log_string_tmp_buffer);
}


void __LOG_ERROR(const wchar_t *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vswprintf_s<sizeof(log_string_tmp_buffer) / sizeof(wchar_t)>(log_string_tmp_buffer, fmt, ap);
	va_end(ap);

	Logger::error(log_string_tmp_buffer);
}


void __LOG_WARN(const wchar_t *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vswprintf_s<sizeof(log_string_tmp_buffer) / sizeof(wchar_t)>(log_string_tmp_buffer, fmt, ap);
	va_end(ap);

	Logger::warn(log_string_tmp_buffer);
}


void __LOG_INFO(const wchar_t *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vswprintf_s<sizeof(log_string_tmp_buffer) / sizeof(wchar_t)>(log_string_tmp_buffer, fmt, ap);
	va_end(ap);

	Logger::info(log_string_tmp_buffer);
}


void __LOG_DEBUG(const wchar_t *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vswprintf_s<sizeof(log_string_tmp_buffer) / sizeof(wchar_t)>(log_string_tmp_buffer, fmt, ap);
	//_vswprintf(log_string_tmp_buffer, fmt, ap);
	//vswprintf(log_string_tmp_buffer, sizeof(log_string_tmp_buffer)/sizeof(wchar_t), fmt, ap);
	va_end(ap);

	Logger::debug(log_string_tmp_buffer);
}
} // extern "C"

