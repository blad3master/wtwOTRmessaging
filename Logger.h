#pragma once

#include "StdAfx.h"

class Logger
{
public:
	/*
	static const unsigned int LOG_CRITICAL = 0x01;
	static const unsigned int LOG_ERROR = 0x02;
	static const unsigned int LOG_WARN = 0x04;
	static const unsigned int LOG_INFO = 0x08;
	static const unsigned int LOG_DEBUG = 0x10;
	static const unsigned int LOG_TRACE = 0x20;
	*/
	enum LEVEL {
		LEVEL_CRITICAL = 0x01,
		LEVEL_ERROR    = 0x02,
		LEVEL_WARN     = 0x04,
		LEVEL_INFO     = 0x08,
		LEVEL_DEBUG    = 0x10,
		LEVEL_TRACE    = 0x20
	};

	explicit Logger(void);
	~Logger(void);

	inline static Logger *getInstance();

	// Open file and duplicate logs there
	void startLoggingToFile();

	static void critical(const wchar_t *msg);
	static void error(const wchar_t *msg);
	static void warn(const wchar_t *msg);
	static void info(const wchar_t *msg);
	static void debug(const wchar_t *msg);
	static void trace(const wchar_t *msg);

private:
	static Logger * instance;
	static unsigned int loggingFlags;

	FILE *logFileDescriptor = nullptr;

	static void logToFile(const wchar_t *msg);
};


inline Logger *Logger::getInstance()
{
	return instance;
}

#if defined(_DEBUG) || !defined(NDEBUG)
#define _WTWOTRMESSAGING_EXTRA_LOGGING 1
#endif


extern "C" {
	void __LOG_CRITICAL(const wchar_t *fmt, ...);
	void __LOG_ERROR(const wchar_t *fmt, ...);
	void __LOG_WARN(const wchar_t *fmt, ...);
	void __LOG_INFO(const wchar_t *fmt, ...);
	void __LOG_DEBUG(const wchar_t *fmt, ...);
	void __LOG_TRACE(const wchar_t *fmt, ...);

	#define LOG_CRITICAL(fmt, ...) __LOG_CRITICAL(L"[CRI] " fmt, __VA_ARGS__)
	#define LOG_ERROR(fmt, ...) __LOG_ERROR(L"[ERR] " fmt, __VA_ARGS__)
	#define LOG_WARN(fmt, ...) __LOG_WARN(L"[WRN] " fmt, __VA_ARGS__)
	#define LOG_INFO(fmt, ...) __LOG_INFO(L"[INF] " fmt, __VA_ARGS__)
	#define LOG_DEBUG(fmt, ...) __LOG_DEBUG(L"[DBG] " fmt, __VA_ARGS__)
	#define LOG_TRACE(fmt, ...) __LOG_TRACE(L"[TRC] " fmt, __VA_ARGS__)
}
