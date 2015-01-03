#include <memory>
#include "StdAfx.h"
#include "Logger.h"
#include "utils.h"

#include <Strsafe.h>

static const wchar_t LOG_PREFIX[] = L"OTR";

static wchar_t log_string_tmp_buffer[4 * 1024];

Logger * Logger::instance = nullptr;
unsigned int Logger::loggingFlags = 0xffffffff;



Logger::Logger(void)
{
	if (nullptr == instance)
	{
		instance = this;
	}
	else
	{
		LOG_CRITICAL(L"%s() Logger instace already exists!", __FUNCTIONW__);
		ASSERT(0); // do not ignore it 
	}
}


Logger::~Logger(void)
{
	if (nullptr != instance)
	{
		if (nullptr != logFileDescriptor)
		{
			LOG_TRACE(L"%s()", __FUNCTIONW__);
			fclose(logFileDescriptor);
			logFileDescriptor = nullptr;
		}

		instance = nullptr;
	}
	else
	{
		LOG_CRITICAL(L"%s() Logger instace does not exists!", __FUNCTIONW__);
		ASSERT(0);
	}
}


void Logger::startLoggingToFile()
{
	if (nullptr == logFileDescriptor)
	{
		const size_t MAX_FILE_PATH_LEN = 255;
		wchar_t *filepath = new wchar_t[MAX_FILE_PATH_LEN];
		std::unique_ptr<wchar_t[]> bufferRelease(filepath);	// make sure buffer is released
		wtwDirectoryInfo di;
		initStruct(di);
		di.flags = WTW_DIRECTORY_FLAG_FULLPATH;
		di.dirType = WTW_DIRECTORY_PLUGINDATA;
		di.bi.bufferSize = MAX_FILE_PATH_LEN;
		di.bi.pBuffer = filepath;

		if (S_OK == wtw->fnCall(WTW_GET_DIRECTORY_LOCATION, reinterpret_cast<WTW_PARAM>(&di), NULL))
		{
			if (0 != wcsncmp(L"", filepath, MAX_FILE_PATH_LEN))
			{
				StringCbPrintfW(filepath, MAX_FILE_PATH_LEN * sizeof(wchar_t), L"%s%s",
					filepath, L"wtwOTRmessaging.log");

				errno_t err = _wfopen_s(&logFileDescriptor, filepath, L"w");
				if (0 != err)
				{
					LOG_CRITICAL(L"%s() Cannot open file '%s' for writing", __FUNCTIONW__, filepath);
					logFileDescriptor = nullptr;
				}

				logToFile(L"Start");
			}
			else
			{
				LOG_CRITICAL(L"%s() Failed to obtain directory location (empty string)!", __FUNCTIONW__);
			}
		}
		else {
			LOG_CRITICAL(L"%s() Failed to obtain directory location (return status)!", __FUNCTIONW__);
			ASSERT(0);
		}
	}
	else
	{
		LOG_CRITICAL(L"%s() Logger file already opened!", __FUNCTIONW__);
		ASSERT(0);
	}
}


void Logger::logToFile(const wchar_t *msg)
{
	if ((nullptr != instance) &&
		(nullptr != instance->logFileDescriptor) &&
		(nullptr != msg))
	{
		SYSTEMTIME time;
		GetSystemTime(&time);
		fprintf_s(instance->logFileDescriptor, "[%u-%02u-%02u %02u:%02u:%02u.%03u] ",
			time.wYear, time.wMonth, time.wDay,
			time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

		fprintf_s(instance->logFileDescriptor, "%s", utf16Toutf8(msg));
		fprintf_s(instance->logFileDescriptor, "\n");
		fflush(instance->logFileDescriptor);
	}
}


void Logger::critical(const wchar_t *msg)
{
	logToFile(msg);
	if (LEVEL::LEVEL_CRITICAL & loggingFlags)
	{
		__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_CRITICAL);
	}
}


void Logger::error(const wchar_t *msg)
{
	logToFile(msg);
	if (LEVEL::LEVEL_ERROR & loggingFlags) {
		__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_ERROR);
	}
}


void Logger::warn(const wchar_t *msg) {
	logToFile(msg);
	if (LEVEL::LEVEL_WARN & loggingFlags)
	{
		__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_ERROR);
	}
}


void Logger::info(const wchar_t *msg)
{
	logToFile(msg); 
	if (LEVEL::LEVEL_INFO & loggingFlags)
	{
		__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_INFO);
	}
}


void Logger::debug(const wchar_t *msg)
{
	logToFile(msg);
	if (LEVEL::LEVEL_DEBUG & loggingFlags)
	{
		__LOG(wtw, LOG_PREFIX, msg, WTW_LOG_LEVEL_NORMAL);
	}
}

void Logger::trace(const wchar_t *msg)
{
	logToFile(msg);
	if (LEVEL::LEVEL_TRACE & loggingFlags)
	{
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

void __LOG_TRACE(const wchar_t *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vswprintf_s<sizeof(log_string_tmp_buffer) / sizeof(wchar_t)>(log_string_tmp_buffer, fmt, ap);
	va_end(ap);
	Logger::trace(log_string_tmp_buffer);
}
} // extern "C"

