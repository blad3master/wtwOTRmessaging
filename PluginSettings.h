#pragma once

#include <string>
#include "wtwOTRmessaging.h"

class PluginSettings
{
public:

	struct key {
		static const wchar_t * test_key;
	};


	PluginSettings();
	~PluginSettings();

	// on plugin load
	static void init();

	// on plugin unload
	static void destroy();

	static bool readFromFile();

	static bool writeToFile();

	static std::wstring getString(const wchar_t *key, const wchar_t *defaultVal);

	static bool setString(const wchar_t *key, const wchar_t *value);

	static int getInt(const wchar_t *key, const int defaultVal);

	static bool setInt(const wchar_t *key, const int value);

private:
	static void test_read();

	static void test_write();

	static void * pSettingsInterface;
};


