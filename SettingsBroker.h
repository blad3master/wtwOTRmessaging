#pragma once

#include <vector>
#include "wtwapi/wtwOptions.h"
#include "Logger.h"

class wtwOTRmessaging;

class SettingsBroker
{
public:
	enum class OTRL_POLICY : unsigned int {
		NEVER,
		MANUAL,
		OPPORTUNISTIC,
		ALWAYS
	};

	SettingsBroker(wtwOTRmessaging*);
	~SettingsBroker(void);

	/* Class getters & setters */
	const wchar_t * getSettingsDataDir() const;
	const wchar_t * getSettingsFileFullPath() const;
	const wchar_t * getPrivateKeyFullPath() const;
	const wchar_t * getInstagFileFullPath() const;
	const wchar_t * getFigerprintFileFullPath() const;

	// used for netClass in wtw structs
//	const wchar_t * getProtocolNetClass() const;
//	inline void setOTRLprotocol(const char *protocol);

	// used for netId in wtw structs
//	int getProtocolNetId() const;
	//	inline void setOTRLaccountName(const char *accountName);

	const char * getOtrlAccountName() const;

	OTRL_POLICY getOtrlPolicy() const;

	void ui_update_keylist();


	// Settings Instance
	static SettingsBroker * settingsBrokerInstance;

private:
	static const unsigned int MAX_SETTING_STRING_LEN = 512;
	static const unsigned int MAX_FILE_PATH_LEN = 200;

	static const int UI_SETTINGS_WIDTH = 730; // in px
	static const int UI_SETTINGS_HEIGHT = 470; // in px
	enum UI_SETTINGS_HMENU {
		UI_SETTING_HMENU_1					= 1,
		UI_SETTING_HMENU_VERIFY_KEY,
		UI_SETTING_HMENU_FORGET_KEY,
		UI_SETTING_HMENU_LOG_TO_FILE		= 100,
		UI_SETTING_HMENU_ARCHIVE_MESSAGES
	};

	static const wchar_t * const SETTINGS_FILE_NAME;
	static const wchar_t * const PRIV_KEY_FILE_NAME;
	static const wchar_t * const INSTAG_FILE_NAME;
	static const wchar_t * const FINGERPRINT_FILE_NAME;
	static const wchar_t * const SettingsWindowClassName; // Class name used to register settings window

	enum SETTING_KEY {
		SETTING_KEY_DATA_DIR,				// Directory where plugin settings will be kept
		SETTING_KEY_SETTINGS_FILE_PATH,		// Full path to the settings file
		SETTING_KEY_PRIV_KEY_FILE_PATH,
		SETTING_KEY_INSTAG_FILE_PATH,
		SETTING_KEY_FINGERPRINT_FILE_PATH,
//		SETTING_KEY_PROTOCOL_NET_CLASS,
//		SETTING_KEY_PROTOCOL_NET_ID,
//		SETTING_KEY_OTRL_ACCOUNT_NAME,
		SETTING_KEY_OTRL_POLICY,
		SETTING_KEY_LOG_TO_FILE,
		SETTING_KEY_NUMBER_OF_KEYS			// used for boundary checks
	};

	enum SETTING_TYPE {
		SETTING_TYPE_INVALID,
		SETTING_TYPE_BOOL,
		SETTING_TYPE_INT,
		SETTING_TYPE_UINT,
		SETTING_TYPE_C_STRING,
		SETTING_TYPE_WC_STRING
	};

	struct SETTING {
		SETTING_TYPE type;
		union {
			bool value_bool;
			int value_int;
			unsigned int value_uint;
			char value_c_string[MAX_SETTING_STRING_LEN];
			wchar_t value_wc_string[MAX_SETTING_STRING_LEN];
		};
	};

	static const wchar_t * const SETTINGS_WTW_NAMES[SETTING_KEY_NUMBER_OF_KEYS];

	// array of all settings available
	SETTING settingsMap[SETTING_KEY_NUMBER_OF_KEYS];


/*
	struct UI_ELEMENT {
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
		HWND hwnd = 0;
		UI_ELEMENT(int a, int b, int c, int d, HWND e) : x(a), y(b), w(c), h(d), hwnd(e) {};
	};
*/

	/**************/
	/* UI hadlers */
	/**************/

	// wtwOTRmessaging settings window
	static HWND ui_settingsWnd;

	// current scroll applied to ui_settingsWnd
	static POINT ui_settingsWnd_scroll;

	// vector of all ui elements
//	static std::vector<UI_ELEMENT> ui_elements;

	// wtwOTRmessaging settings window - full file path
	HWND ui_settingsFilePath;

	// Private key - full file path
	HWND ui_privateKeyFilePath;

	// OTRL policy combobox
	HWND ui_otrl_policy;

	// Enable logging to file?
	HWND ui_log_to_file;

	// List of keys
	HWND ui_keyList = 0;
	std::vector<void*> keyList_fingerprints; // mapping of ui_keyList indexes to ConnContext->fingerprint


	// Pointer to main class
	wtwOTRmessaging * itsWtwOTRmessaging;

private:
	// forget the peer fingerprint (key)
	static void ui_forget_fingerprint();


private:
	// Show options
	WTW_PTR onActionShow(wtwOptionPageShowInfo*);

	WTW_PTR onActionHide(wtwOptionPageShowInfo*);

	WTW_PTR onActionOk(wtwOptionPageShowInfo*);

	WTW_PTR onActionApply(wtwOptionPageShowInfo*);

	WTW_PTR onActionCancel(wtwOptionPageShowInfo*);

	WTW_PTR onActionMove(wtwOptionPageShowInfo*);

	WTW_PTR onActionReload(wtwOptionPageShowInfo*);

	bool getSettingValueHelper(SETTING_KEY key, SETTING_TYPE type, void* value) const;

	static LRESULT CALLBACK settings_class_callback(HWND, UINT, WPARAM, LPARAM);

	static LRESULT CALLBACK settings_class_WNDPROC(HWND, UINT, WPARAM, LPARAM);

	static WTW_PTR settings_callback(WTW_PARAM wParam /*wtwOptionPageShowInfo*/, WTW_PARAM lParam, void* ptr);

	void setSettingsBrokerInstancePointer();

	void unsetSettingsBrokerInstancePointer();

	void makeSettingsFilesPaths();

	void addSettingsPage();

	void removeSettingsPage();

	void registerWndClass();

	void initializeSettingsValues();

//	void createSettingsIterface();

//	void destroySettingsInterface();

	void loadSettingsFromFile();
	
	void saveSettingsToFile();
};


