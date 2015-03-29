#include "stdafx.h"
#include "PluginSettings.h"
#include "wtwapi\wtwPluginSettings.h"
#include "utils.h"
#include <memory>


// decide wheter settings are saved at the moment of theis change, or once while plugin is unloaded
// 0 => write at the end, 1 => write at every change
#define _PLUGINSETTINGS_WRITE_INSTANTLY 1


// decide wheter WTW settings get/set is working as expected
// 0 => NO, 1 => YES
#ifdef _WTWOTRMESSAGING_EXTRA_LOGGING
#define _PLUGINSETTINGS_TEST_WTW_INTERFACE 1
#endif

const wchar_t * PluginSettings::key::test_key = L"key01";

void * PluginSettings::pSettingsInterface = nullptr;


PluginSettings::PluginSettings()
{
	// interface should not be set yet
	if (nullptr != pSettingsInterface)
	{
		LOG_CRITICAL(L"%s() pSettingsInterface is not nullptr!", __FUNCTIONW__);
		pSettingsInterface = nullptr;
	}
}


PluginSettings::~PluginSettings()
{
	// interface should be already destroyed
	if (nullptr != pSettingsInterface)
	{
		LOG_CRITICAL(L"%s() pSettingsInterface is not nullptr!", __FUNCTIONW__);
		pSettingsInterface = nullptr;
	}
}


void PluginSettings::init()
{
	wchar_t *filepath = new wchar_t[MAX_PATH+1];
	std::unique_ptr<wchar_t[]> bufferRelease(filepath);	// make sure buffer is released
	wtwDirectoryInfo di;
	di.flags = WTW_DIRECTORY_FLAG_FULLPATH;
	di.dirType = WTW_DIRECTORY_PLUGINDATA;
	di.bi.bufferSize = MAX_PATH;
	di.bi.pBuffer = filepath;
	
	if (S_OK == wtwPf->fnCall(WTW_GET_DIRECTORY_LOCATION, reinterpret_cast<WTW_PARAM>(&di), NULL))	
	{
		std::wstring configFile = filepath;
		configFile += (*(configFile.end() - 1) != L'\\') ? L"\\" : L"";
		configFile += L"wtwOTRmessaging.config";
		LOG_DEBUG(L"%s() WTW_SETTINGS_GET_MY_CONFIG_FILE: '%s'", __FUNCTIONW__, configFile.c_str());

		if (S_OK == wtwPf->fnCall(WTW_SETTINGS_INIT_EX,
			reinterpret_cast<WTW_PARAM>(configFile.c_str()),
			reinterpret_cast<WTW_PARAM>(&pSettingsInterface)))
		{
			LOG_DEBUG(L"%s() PluginSettings interface created OK", __FUNCTIONW__);

			if (true == readFromFile()) {
				test_read();
			}
		}
		else {
			LOG_CRITICAL(L"%s() PluginSettings initialization FAILED (0x%p)", __FUNCTIONW__, pSettingsInterface);
		}
	}
	else
	{
		pSettingsInterface = nullptr;
		LOG_CRITICAL(L"%s() GET_MY_CONFIG_FILE failed", __FUNCTIONW__);
	}
}


void PluginSettings::destroy()
{
	if (nullptr != pSettingsInterface)
	{
		test_write();

		#if !(_PLUGINSETTINGS_WRITE_INSTANTLY)
		writeToFile();
		#endif

		if (S_OK != wtwPf->fnCall(WTW_SETTINGS_DESTROY, reinterpret_cast<WTW_PARAM>(pSettingsInterface), NULL)) {
			LOG_ERROR(L"%s() WTW_SETTINGS_DESTROY failed", __FUNCTIONW__);
		}
		pSettingsInterface = nullptr;
	}
	else {
		LOG_CRITICAL(L"%s() pSettingsInterace is nullptr", __FUNCTIONW__);
	}
}

bool PluginSettings::readFromFile()
{
	if (nullptr != pSettingsInterface)
	{
		if (1 == wtwPf->fnCall(WTW_SETTINGS_READ, reinterpret_cast<WTW_PARAM>(pSettingsInterface), NULL))
		{
			LOG_DEBUG(L"%s() Read settings from file successfully", __FUNCTIONW__);

			return true;
		}
		else
		{
			LOG_WARN(__FUNCTIONW__ L" Reading settings from file failed (may appear if run for the first time)!");


			// TODO: display ask messagebox:
			// "Nie mozna odczytaæ ustawien wtyczki z pliku - czy utworzyæ domyœlne ustawienia?"
			//if (false == setString(L"wtwOTRmessaging_test_key", L"wtwOTRmessaging_test_value")) {
			//	LOG_CRITICAL(L"%s() failed to save test value");
			//}
		}
	}
	else {
		LOG_CRITICAL(L"%s() pSettingsInterface = nullptr!", __FUNCTIONW__);
	}

	return false;
}


bool PluginSettings::writeToFile()
{
	if (nullptr != pSettingsInterface)
	{
		if (1 == wtwPf->fnCall(WTW_SETTINGS_WRITE, reinterpret_cast<WTW_PARAM>(pSettingsInterface), NULL))
		{
			LOG_DEBUG(L"%s() Settings saved to file successfully", __FUNCTIONW__);

			return true;
		}
		else {
			LOG_ERROR(L"%s() Saving settings to file failed!", __FUNCTIONW__);
		}
	}
	else {
		LOG_CRITICAL(L"%s() pSettingsInterface = nullptr!", __FUNCTIONW__);
	}

	return false;
}


std::wstring PluginSettings::getString(const wchar_t *key, const wchar_t *defaultVal)
{
	if ((nullptr != pSettingsInterface) && (nullptr != wtwPf))
	{
		wchar_t *buffer = nullptr;
		
		if (1 == wtwGetStr(wtwPf, pSettingsInterface, key, defaultVal, &buffer))
		{
			LOG_TRACE(L"%s() read settings ok: '%s' => '%s'", __FUNCTIONW__, key, buffer);

			std::wstring val = buffer;
			delete [] buffer;
			return val;
		}
		else {
			LOG_ERROR(L"%s() read settings failed: key = '%s'", __FUNCTIONW__, key);
		}
	}
	else
	{
		LOG_CRITICAL(L"%s() No interface to reload settings!", __FUNCTIONW__);
	}

	return defaultVal;
}

bool PluginSettings::setString(const wchar_t *key, const wchar_t *value)
{
	LOG_TRACE(L"%s() setting '%s' => '%s'...", __FUNCTIONW__, key, value);

	if ((nullptr != pSettingsInterface) && (nullptr != wtwPf))
	{
		// WTW API BUG (as of api-20150314)
		// CAUTION: WTW API says that WTW_SETTINGS_SET_VALUE returns 1 on success, 0 on error
		// but my tests shows that on success it returns 0, and non-zero on error!
		if (0 == wtwSetStr(wtwPf, pSettingsInterface, key, value))
		{
			#if _PLUGINSETTINGS_WRITE_INSTANTLY
			if (true == writeToFile()) {
				LOG_TRACE(L"%s() SET '%s' => '%s'", __FUNCTIONW__, key, value);
				return true;
			}
			else {
				LOG_ERROR(L"%s() Saving settings to file failed!", __FUNCTIONW__);
			}
			#endif
		}
		else {
			LOG_ERROR(L"%s() - wtwSetStr failed", __FUNCTIONW__);
		}
	}
	else {
		LOG_CRITICAL(L"%s() pSettingsInterface = nullptr", __FUNCTIONW__);
	}

	return false;
}


int PluginSettings::getInt(const wchar_t *key, const int defaultVal)
{
	if ((nullptr != pSettingsInterface) && (nullptr != wtwPf))
	{
		return wtwGetInt(wtwPf, pSettingsInterface, key, defaultVal);
	}
	else
	{
		LOG_CRITICAL(L"%s() No interface to reload settings!", __FUNCTIONW__);
	}

	return defaultVal;
}


bool PluginSettings::setInt(const wchar_t *key, const int value)
{
	LOG_TRACE(L"%s() setting '%s' => '%d'...", __FUNCTIONW__, key, value);

	if ((nullptr != pSettingsInterface) && (nullptr != wtwPf))
	{
		if (value == wtwSetInt(wtwPf, pSettingsInterface, key, value))
		{
#if _PLUGINSETTINGS_WRITE_INSTANTLY
			if (true == writeToFile()) {
				LOG_TRACE(L"%s() SET '%s' => '%d'", __FUNCTIONW__, key, value);
				return true;
			}
			else {
				LOG_ERROR(L"%s() Saving settings to file failed!", __FUNCTIONW__);
			}
#endif
		}
		else {
			LOG_ERROR(L"%s() - wtwSetInt failed", __FUNCTIONW__);
		}
	}
	else {
		LOG_CRITICAL(L"%s() pSettingsInterface = nullptr", __FUNCTIONW__);
	}

	return false;
}


void PluginSettings::test_read()
{
#ifdef _PLUGINSETTINGS_TEST_WTW_INTERFACE
	LOG_TRACE(L"%s() testing WTW settings... START", __FUNCTIONW__);

	if (getString(L"wtwOTRmessaging_test_key", L"") != L"wtwOTRmessaging_test_value")
	{
		LOG_CRITICAL(L"%s() getString() failed", __FUNCTIONW__);
	}
	else {
		LOG_TRACE(L"%s() getString() OK", __FUNCTIONW__);
	}

	if (getString(L"", L"some_default_value") != L"some_default_value")
	{
		LOG_CRITICAL(L"%s() getString() failed", __FUNCTIONW__);
	}
	else {
		LOG_TRACE(L"%s() getString() OK", __FUNCTIONW__);
	}

	LOG_TRACE(L"%s() testing WTW settings... END", __FUNCTIONW__);
#endif
}


void PluginSettings::test_write()
{
#ifdef _PLUGINSETTINGS_TEST_WTW_INTERFACE
	LOG_TRACE(L"%s() testing WTW settings... START", __FUNCTIONW__);

	if (false == setString(L"wtwOTRmessaging_test_key", L"wtwOTRmessaging_test_value"))
	{
		LOG_CRITICAL(L"%s() setString() failed", __FUNCTIONW__);
	}
	else {
		LOG_TRACE(L"%s() setString() OK", __FUNCTIONW__);
	}

	if (true == setString(L"", L""))
	{
		LOG_CRITICAL(L"%s() setString() failed", __FUNCTIONW__);
	}
	else {
		LOG_TRACE(L"%s() setString() OK", __FUNCTIONW__);
	}

	LOG_TRACE(L"%s() testing WTW settings... END", __FUNCTIONW__);
#endif
}