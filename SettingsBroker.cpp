#include "StdAfx.h"
#include "SettingsBroker.h"
#include "wtwOTRmessaging.h"
#include "wtwapi/wtwPluginSettings.h"

#include <Strsafe.h> // StringCbPrintfW()
#include "utils.h"

extern "C" {
#include "OTRL/privkey.h"
}

static const wchar_t * WTWOTR_OPTIONS_PAGE_ID = L"wtwOTRmessagingOptions";
const wchar_t * const SettingsBroker::SETTINGS_FILE_NAME = L"wtwOTRmessaging.dat";
const wchar_t * const SettingsBroker::INSTAG_FILE_NAME = L"instag.dat";
const wchar_t * const SettingsBroker::FINGERPRINT_FILE_NAME = L"fingerprint.dat";
const wchar_t * SettingsBroker::SettingsWindowClassName = L"SettingsClass";

const wchar_t * const SettingsBroker::SETTINGS_WTW_NAMES[SETTING_KEY_NUMBER_OF_KEYS] = {
	L"data_dir",				// SETTING_KEY_DATA_DIR
	L"settings_file_path",		// SETTING_KEY_SETTINGS_FILE_PATH
	L"priv_key_file_path",		// SETTING_KEY_PRIV_KEY_FILE_PATH
	L"instag_file_path",		// SETTING_KEY_INSTAG_FILE_PATH
	L"fingerprint_file_path",	// SETTING_KEY_FINGERPRINT_FILE_PATH
};

SettingsBroker * SettingsBroker::settingsBrokerInstance = 0;
HWND SettingsBroker::ui_settingsWnd = 0;
POINT SettingsBroker::ui_settingsWnd_scroll = { 0, 0 };


		#pragma warning(disable: 4995) /*  'wsprintfA': name was marked as #pragma deprecated */
		#include <atlwin.h>
		#pragma warning(default: 4995)
		#include "resource.h"
		HWND parent_settingsWnd = 0;
		SCROLLINFO si = { 0 };  // scroll info structure
		INT iXCurrentScroll = 0;  // current horizontal scroll value
		INT iYCurrentScroll = 0;   // current vertical scroll value




SettingsBroker::SettingsBroker(wtwOTRmessaging *_wtwOTRm) :
	itsWtwOTRmessaging(_wtwOTRm),
	pSettingsInterface(0)
{
	memset(settingsMap, 0, sizeof(settingsMap));
	setSettingsBrokerInstancePointer();
	makeSettingsFilesPaths();
	addSettingsPage();
	registerWndClass();
	initializeSettingsValues();

	// If settings directory does not exists, create it
	DWORD dwAttrib = GetFileAttributes( getSettingsDataDir() );
	if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		if (0 != CreateDirectory(getSettingsDataDir(), NULL)) {
			LOG_INFO(L"Created directory: %s", getSettingsDataDir());
		}
		else {
			LOG_ERROR(L"Failed to create directory: %s", getSettingsDataDir());
			goto _SettingsBroker_end;
		}
	}

	// If settings file does not exists, create it
	#if 0
	dwAttrib = GetFileAttributes( getSettingsFileFullPath() );
	if (dwAttrib == INVALID_FILE_ATTRIBUTES)
	{
		if (INVALID_HANDLE_VALUE != CreateFile(getSettingsFileFullPath(), GENERIC_READ | GENERIC_WRITE,
												0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL) ) {
			LOG_INFO(L"Created file: %s", getSettingsFileFullPath());
		}
		else {
			LOG_ERROR(L"Failed to create file: %s", getSettingsFileFullPath());
			goto _SettingsBroker_end;
		}
	}
	#endif
		
	// TODO: if file does not exists then create it and save with default values,
	//       if file is empty or corrupted, then onActionReload() is not executed and values are not set
	//       for later use!

	createSettingsIterface();
	return;

_SettingsBroker_end:
	LOG_CRITICAL(L"SettingsBroker::SettingsBroker() failure");
}

SettingsBroker::~SettingsBroker(void)
{
	destroySettingsInterface();
	removeSettingsPage();
	unsetSettingsBrokerInstancePointer();
}


const wchar_t * SettingsBroker::getSettingsDataDir() const
{
	if (SETTING_TYPE_WC_STRING == settingsMap[SETTING_KEY_DATA_DIR].type) {
		//LOG_DEBUG(L"SettingsBroker::getSettingsDataDir(): %s", settingsMap[SETTING_KEY_DATA_DIR].value_wc_string);
		return settingsMap[SETTING_KEY_DATA_DIR].value_wc_string;
	}

	LOG_CRITICAL(L"SettingsBroker::getSettingsDataDir() - invalid setting type");
	return L"";
}


const wchar_t * SettingsBroker::getSettingsFileFullPath() const
{
	if (SETTING_TYPE_WC_STRING == settingsMap[SETTING_KEY_SETTINGS_FILE_PATH].type) {
		//LOG_DEBUG(L"getSettingsFileFullPath(): %s", settingsMap[SETTING_KEY_SETTINGS_FILE_PATH].value_wc_string);
		return settingsMap[SETTING_KEY_SETTINGS_FILE_PATH].value_wc_string;
	}

	LOG_CRITICAL(L"getSettingsFileFullPath() - invalid setting type");
	return L"";
}

const wchar_t * SettingsBroker::getPrivateKeyFullPath() const
{
	if (SETTING_TYPE_WC_STRING == settingsMap[SETTING_KEY_PRIV_KEY_FILE_PATH].type) {
		//LOG_DEBUG(L"getPrivateKeyFullPath(): %s", settingsMap[SETTING_KEY_PRIV_KEY_FILE_PATH].value_wc_string);
		return settingsMap[SETTING_KEY_PRIV_KEY_FILE_PATH].value_wc_string;
	}

	LOG_CRITICAL(L"getPrivateKeyFullPath() - invalid setting type");
	return L"";
}


const wchar_t * SettingsBroker::getInstagFileFullPath() const
{
	if (SETTING_TYPE_WC_STRING == settingsMap[SETTING_KEY_INSTAG_FILE_PATH].type) {
		//LOG_DEBUG(L"getInstagFileFullPath(): %s", settingsMap[SETTING_KEY_INSTAG_FILE_PATH].value_wc_string);
		return settingsMap[SETTING_KEY_INSTAG_FILE_PATH].value_wc_string;
	}

	LOG_CRITICAL(L"getInstagFileFullPath() - invalid setting type");
	return L"";
}

const wchar_t * SettingsBroker::getFigerprintFileFullPath() const
{
	if (SETTING_TYPE_WC_STRING == settingsMap[SETTING_KEY_FINGERPRINT_FILE_PATH].type) {
		//LOG_DEBUG(L"getFigerprintFileFullPath(): %s", settingsMap[SETTING_KEY_FINGERPRINT_FILE_PATH].value_wc_string);
		return settingsMap[SETTING_KEY_FINGERPRINT_FILE_PATH].value_wc_string;
	}

	LOG_CRITICAL(L"getFigerprintFileFullPath() - invalid setting type");
	return L"";
}


const wchar_t * SettingsBroker::getProtocolNetClass() const
{
	if (SETTING_TYPE_WC_STRING == settingsMap[SETTING_KEY_PROTOCOL_NET_CLASS].type) {
		//LOG_DEBUG(L"getProtocolNetClass(): %s",settingsMap[SETTING_KEY_PROTOCOL_NET_CLASS].value_wc_string);
		return settingsMap[SETTING_KEY_PROTOCOL_NET_CLASS].value_wc_string;
	}

	LOG_CRITICAL(L"%s() - invalid setting type", utf8Toutf16(__FUNCTION__));
	return L"";
}

/*
inline void SettingsBroker::setOTRLaccountName(const char *accountName) {
	if (accountName) {
		const int len = sizeof(settingsMap[SETTING_KEY_ACCOUNT_NAME].value_c_string);
		strncpy(settingsMap[SETTING_KEY_ACCOUNT_NAME].value_c_string, accountName, len);
		settingsMap[SETTING_KEY_ACCOUNT_NAME].value_c_string[len - 1] = 0;
	}
	else {
		LOG_CRITICAL(L"SettingsBroker::setOTRLaccountName() NULL ptr");
	}
}*/


int SettingsBroker::getProtocolNetId() const
{
	if (SETTING_TYPE_INT == settingsMap[SETTING_KEY_PROTOCOL_NET_ID].type) {
		//LOG_DEBUG(L"getProtocolNetId(): %d",settingsMap[SETTING_KEY_PROTOCOL_NET_ID].value_int);
		return settingsMap[SETTING_KEY_PROTOCOL_NET_ID].value_int;
	}

	LOG_CRITICAL(L"%s() - invalid setting type", utf8Toutf16(__FUNCTION__));
	return -1;
}

/*
inline void SettingsBroker::setOTRLprotocol(const char *protocol) {
	if (protocol) {
		const int len = sizeof(settingsMap[SETTING_KEY_PROTOCOL].value_c_string);
		strncpy(settingsMap[SETTING_KEY_PROTOCOL].value_c_string, protocol, len);
		settingsMap[SETTING_KEY_PROTOCOL].value_c_string[len - 1] = 0;
	}
	else {
		LOG_CRITICAL(L"SettingsBroker::setOTRLprotocol() NULL ptr");
	}
}
*/


const char * SettingsBroker::getOtrlAccountName() const
{
	if (SETTING_TYPE_C_STRING == settingsMap[SETTING_KEY_OTRL_ACCOUNT_NAME].type) {
		//LOG_DEBUG(L"getProtocolNetClass(): %s",cToWideCharString(settingsMap[SETTING_KEY_OTRL_ACCOUNT_NAME].value_c_string));
		return settingsMap[SETTING_KEY_OTRL_ACCOUNT_NAME].value_c_string;
	}

	LOG_CRITICAL(L"%s() - invalid setting type", utf8Toutf16(__FUNCTION__));
	return "";
}


LRESULT CALLBACK SettingsBroker::settings_class_callback(HWND hwnd,	UINT msg, WPARAM wParam, LPARAM lParam)
{
	// treat mouse wheel as usual scrollbar event
	if (msg == WM_MOUSEWHEEL)
	{
		msg = WM_VSCROLL;
		int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (0 < delta) {
			wParam = SB_PAGEUP;
		}
		else {
			wParam = SB_PAGEDOWN;
		}
	}

	switch (msg)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case 1:
			MessageBox( nullptr, L"Wcisn¹³eœ przycisk 1",L"", MB_ICONINFORMATION );
			//settingsBrokerInstance->onGeneratePrivateKey();
			break;
		case 2:
			//LOG_DEBUG(L"hot item index = %d", ListView_GetNextItem(settingsBrokerInstance->ui_keyList, -1, LVNI_SELECTED));
			// TODO: verify key again
			break;
		case 3:
			// Forget the key
			ui_forget_fingerprint();
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		DestroyWindow(hwnd);
		//PostQuitMessage( 0 );
		break;
	case WM_CTLCOLORSTATIC:
		{
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);
		static HBRUSH hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));
		return (INT_PTR)hbrBkgnd;

		/*HDC hdcStatic = (HDC) wParam;
		SetTextColor(hdcStatic, RGB(199,0,0));
		SetBkMode (hdcStatic, TRANSPARENT);
		return (LRESULT)GetStockObject(NULL_BRUSH);*/

		/*HBRUSH hbr = (HBRUSH)DefWindowProc(hwnd, msg, wParam, lParam);
		::DeleteObject(hbr);
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (LRESULT)::GetStockObject(NULL_BRUSH);
		*/
		}
		break;
	//case WM_PARENTNOTIFY:
	//case WM_WINDOWPOSCHANGED:
		//LOG_DEBUG(L"SettingsBroker::settings_class_callback() - not implemented");
		break;


	case WM_SIZE:
		// when window is resized, scroll window according to its current size
		SendMessage(hwnd, WM_HSCROLL, SB_ENDSCROLL, 0);
		SendMessage(hwnd, WM_VSCROLL, SB_ENDSCROLL, 0);
		break;

	case WM_HSCROLL:
		{
			int nScrollCode = (int)LOWORD(wParam); // scroll bar value 
			int nPos = (short int)HIWORD(wParam); // scroll box position 
			HWND hwndScrollBar = (HWND)lParam; // handle of scroll bar

			switch (nScrollCode)
			{
			case SB_LINELEFT:
				nPos = GetScrollPos(ui_settingsWnd, SB_HORZ) - 1;
				break;
			case SB_LINERIGHT:
				nPos = GetScrollPos(ui_settingsWnd, SB_HORZ) + 1;
				break;
			case SB_PAGELEFT:
				nPos = GetScrollPos(ui_settingsWnd, SB_HORZ) - 10;
				break;
			case SB_PAGERIGHT:
				nPos = GetScrollPos(ui_settingsWnd, SB_HORZ) + 10;
				break;
			case SB_THUMBTRACK:
				// nPos alread calculated
				break;
			case SB_ENDSCROLL:
				nPos = GetScrollPos(ui_settingsWnd, SB_HORZ);
				break;
			default:
				// do not let below code execute
				return 0;
			}

			RECT rParent;
			GetWindowRect(ui_settingsWnd, &rParent);
			POINT b = { rParent.right, rParent.bottom };
			ScreenToClient(ui_settingsWnd, &b);

			// make sure we are in allowed range
			nPos = (nPos > 0) ? nPos : 0;
			nPos = (nPos < 100) ? nPos : 100;

			// do not scroll right when parent is big enough to house the child
			int new_x = min(0, int((b.x - UI_SETTINGS_WIDTH - 25) * (nPos / 100.0)));
			int diff = new_x - ui_settingsWnd_scroll.x;
			ui_settingsWnd_scroll.x += diff;

			ScrollWindow(ui_settingsWnd, diff, 0, nullptr, nullptr);
			SetScrollPos(ui_settingsWnd, SB_HORZ, nPos, TRUE);
			UpdateWindow(ui_settingsWnd);
		}
		break;
	case WM_VSCROLL:
		{
			int nScrollCode = (int)LOWORD(wParam); // scroll bar value 
			int nPos = (short int)HIWORD(wParam); // scroll box position 
			HWND hwndScrollBar = (HWND)lParam; // handle of scroll bar

			switch (nScrollCode)
			{
			case SB_LINEUP:
				nPos = GetScrollPos(ui_settingsWnd, SB_VERT) - 1;
				break;
			case SB_LINEDOWN:
				nPos = GetScrollPos(ui_settingsWnd, SB_VERT) + 1;
				break;
			case SB_PAGEUP:
				nPos = GetScrollPos(ui_settingsWnd, SB_VERT) - 10;
				break;
			case SB_PAGEDOWN:
				nPos = GetScrollPos(ui_settingsWnd, SB_VERT) + 10;
				break;
			case SB_THUMBTRACK:
				// nPos alread calculated
				break;
			case SB_ENDSCROLL:
				nPos = GetScrollPos(ui_settingsWnd, SB_VERT);
				break;
			default:
				// do not let below code execute
				return 0;
			}

			RECT rParent = { 0 };
			GetWindowRect(ui_settingsWnd, &rParent);
			POINT b = { rParent.right, rParent.bottom };
			ScreenToClient(ui_settingsWnd, &b);

			// make sure we are in allowed range
			nPos = (nPos > 0) ? nPos : 0;
			nPos = (nPos < 100) ? nPos : 100;

			// do not scroll right when parent is big enough to house the child
			int new_y = min(0, int((b.y - UI_SETTINGS_HEIGHT - 25) * (nPos / 100.0)));
			int diff = new_y - ui_settingsWnd_scroll.y;
			ui_settingsWnd_scroll.y += diff;

			ScrollWindow(ui_settingsWnd, 0, diff, nullptr, nullptr);
			SetScrollPos(ui_settingsWnd, SB_VERT, nPos, TRUE);
			UpdateWindow(ui_settingsWnd);
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

LRESULT CALLBACK SettingsBroker::settings_class_WNDPROC(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		LOG_DEBUG(L"settings_class_WNDPROC - WM_SIZE not implemented");
		break;
	default:
		//LOG_DEBUG(L"settings_class_WNDPROC - DEFAULT not implemented");
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}


WTW_PTR SettingsBroker::settings_callback(WTW_PARAM wParam /*wtwOptionPageShowInfo*/, WTW_PARAM lParam, void* ptr)
{
	wtwOptionPageShowInfo* p_wtwOptionPageShowInfo = reinterpret_cast<wtwOptionPageShowInfo*>(wParam);
	SettingsBroker* p_Setting = reinterpret_cast<SettingsBroker*>(ptr);

	switch (p_wtwOptionPageShowInfo->action) {
	case WTW_OPTIONS_PAGE_ACTION_SHOW:
		return p_Setting->onActionShow(p_wtwOptionPageShowInfo);
		break;
	case WTW_OPTIONS_PAGE_ACTION_HIDE:
		return p_Setting->onActionHide(p_wtwOptionPageShowInfo);
		break;
	case WTW_OPTIONS_PAGE_ACTION_OK:
		// ********************************
		// UWAGA: Po OK i Cancel, jesli
		// stworzylismy okno ustawien
		// to powinnismy je zniszczyc (!)
		// ********************************
		return p_Setting->onActionOk(p_wtwOptionPageShowInfo);
		break;
	case WTW_OPTIONS_PAGE_ACTION_APPLY:
		return p_Setting->onActionApply(p_wtwOptionPageShowInfo);
		break;
	case WTW_OPTIONS_PAGE_ACTION_CANCEL:
		// ********************************
		// UWAGA: Po OK i Cancel, jesli
		// stworzylismy okno ustawien
		// to powinnismy je zniszczyc (!)
		// ********************************
		return p_Setting->onActionCancel(p_wtwOptionPageShowInfo);
		break;
	case WTW_OPTIONS_PAGE_ACTION_MOVE:
		return p_Setting->onActionMove(p_wtwOptionPageShowInfo);
		break;
	case WTW_OPTIONS_PAGE_ACTION_RELOAD:	// Re-read settings
		return p_Setting->onActionReload(p_wtwOptionPageShowInfo);
		break;
	default:
		MessageBox(NULL, L"Unexpected action", L"SettingsBroker::settings_callback()", MB_OK);
	};

	//MessageBox(NULL, buf, L"Parametry", MB_OK);
	return 1;
}


WTW_PTR SettingsBroker::onActionShow(wtwOptionPageShowInfo* pInfo)
{
	//MessageBox(NULL, L"onActionShow", L"Parametry", MB_OK);

	#if POLISH
	CString caption = L"wtwOTRmessaging - implementacja protoko³u Off-the-Record Messaging";
	CString description = L"Wtyczka pozwala uwierzytelniæ naszego rozmówcê i szyfrowaæ wymieniane wiadomoœci.";
	CString settings_group_name = L"Ustawienia ogólne";
	CString settings_file = L"Plik ustawieñ:";
	CString cb_start_priv = L" Próbuj nawi¹zaæ prywatne po³¹czenie automatycznie";
	CString cb_force_enc = L" Wymuœ szyfrowanie";
	CString cb_do_archive = L" Zapisuj wiadomoœci do archiwum";
	CString priv_key = L"Twój klucz prywatny";
	CString priv_key_hash = L"Odcisk klucza: %s";
	CString priv_key_file = L"Plik z kluczem:";
	CString generate_priv_key = L"Generuj klucz prywatny";
	CString known_keys = L"Znane klucze";
	#else
	CString caption = L"wtwOTRmessaging - Off-the-Record Messaging protocol plugin";
	CString description = L"This plugin provides authentication, encryption, perfect forward secrecy and malleable encryption.";
	CString settings_group_name = L"General settings";
	CString settings_file = L"Settings file:";
	CString cb_start_priv = L" Automatically start private conversations";
	CString cb_force_enc = L" Force encryption";
	CString cb_do_archive = L" Archive message history";
	CString priv_key = L"Private key";
	CString priv_key_hash = L"Key fingerprint: %s";
	CString priv_key_file = L"Key file:";
	CString generate_priv_key = L"Generate private key";
	CString known_keys = L"Known keys";
	#endif

	wcscpy_s(pInfo->windowCaption, 256, caption);
	wcscpy_s(pInfo->windowDescrip, 512, description);
	// TODO: prepare big icon and set "icon" or "iconId";

	if (nullptr != ui_settingsWnd) {
		ShowWindow(ui_settingsWnd, SW_SHOW);
		UpdateWindow(ui_settingsWnd);
	}
	else {
		ui_settingsWnd = CreateWindowEx(0, SettingsWindowClassName, L"wtwOTRmessaging settings",
			WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
			pInfo->x, pInfo->y, /*pInfo->cx*/ UI_SETTINGS_WIDTH, /*pInfo->cy*/ UI_SETTINGS_HEIGHT,
			pInfo->handle, NULL, hInstance, NULL);

		if (ui_settingsWnd == NULL) {
			MessageBox(NULL, L"Error:\nFailed to create settings window!", L"wtwOtrMessaging", MB_ICONEXCLAMATION);
			return 1;
		}

		char *buffer = new char[255];
		wchar_t *wc_buffer = new wchar_t[255];

		// GroupBox
		CreateWindow(L"BUTTON", settings_group_name, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
			0, 0, 720, 125, ui_settingsWnd, NULL, hInstance, NULL);

		HWND labelFile = CreateWindowEx(0, L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_LEFT,
			5, 25, 90, 20, ui_settingsWnd, NULL, hInstance, NULL);
		SetWindowText(labelFile, settings_file);

		// Edit - settings file path
		ui_settingsFilePath = CreateWindowEx(0, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
			110, 23, 600, 20, ui_settingsWnd, NULL, hInstance, NULL);
		SetWindowText(ui_settingsFilePath, getSettingsFileFullPath());

		// Checkbox
		CreateWindowEx(0, L"BUTTON", cb_start_priv,
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_DISABLED, 8, 50, 550, 20, ui_settingsWnd, NULL, hInstance, NULL);

		// Checkbox
		CreateWindowEx(0, L"BUTTON", cb_force_enc,
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_DISABLED, 8, 75, 550, 20, ui_settingsWnd, NULL, hInstance, NULL);

		// Checkbox
		CreateWindowEx(0, L"BUTTON", cb_do_archive,
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_DISABLED, 8, 100, 550, 20, ui_settingsWnd, NULL, hInstance, NULL);

		// GroupBox
		CreateWindow(L"BUTTON", priv_key, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
			0, 135, 720, 110, ui_settingsWnd, NULL, hInstance, NULL);

		HWND labelFile2 = CreateWindowEx(0, L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_LEFT,
			5, 160, 130, 20, ui_settingsWnd, NULL, hInstance, NULL);
		SetWindowText(labelFile2, priv_key_file);

		// Edit - private key file path
		ui_privateKeyFilePath = CreateWindowEx(0, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
			120, 158, 590, 20, ui_settingsWnd, NULL, hInstance, NULL);
		SetWindowText(ui_privateKeyFilePath, getPrivateKeyFullPath());

		if (otrl_privkey_fingerprint(itsWtwOTRmessaging->getOtrlUserState(),
							buffer, getOtrlAccountName(), utf16Toutf8(getProtocolNetClass())))
		{
			//wcscpy_s(wc_buffer, 255, L"Private key hash: ");
			//size_t len = wcslen(wc_buffer);
			//mbstowcs_s(0, &(wc_buffer[len]), 255 - len, buffer, 255 - len);

			StringCchPrintfW(wc_buffer, 255, priv_key_hash, utf8Toutf16(buffer));
		}
		else
		{
			StringCbPrintfW(wc_buffer, 255 * sizeof(wchar_t), L"No private key hash for %s@%s",
				utf8Toutf16("noName"),
				utf8Toutf16("noProtocol"));
			
			//wcscpy_s(wc_buffer, 255, L"No private key hash");
		}
		// Edit - private key hash
		HWND hash = CreateWindowEx(0, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_READONLY,
			5, 190, 510, 20, ui_settingsWnd, NULL, hInstance, NULL);
		SetWindowText(hash, wc_buffer);

		// Button
		CreateWindowW(L"BUTTON", generate_priv_key, WS_CHILD | WS_VISIBLE | WS_DISABLED,
			5, 215, 190, 25, ui_settingsWnd, (HMENU)1, hInstance, NULL);


		// GroupBox
		CreateWindow(L"BUTTON", known_keys, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
			0, 250, 720, 190, ui_settingsWnd, NULL, hInstance, NULL);

		// List of key hashes
#if 0
		INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
		icex.dwSize = sizeof(icex);
		icex.dwICC = ICC_LISTVIEW_CLASSES;
		InitCommonControlsEx(&icex);
#endif
		ui_keyList = CreateWindow(WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER,
			5, 270, UI_SETTINGS_WIDTH-20, 135, ui_settingsWnd, NULL, hInstance, NULL);
		ListView_SetExtendedListViewStyle(ui_keyList, /* LVS_EX_CHECKBOXES |*/ LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
		{
			LVCOLUMN lvc = { 0 };
			lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
			lvc.fmt = LVCFMT_LEFT;

			/* Add four columns to the list-view (first column contains check box). */
			lvc.iSubItem = 0;
			lvc.cx = 180;
			lvc.pszText = TEXT("Nazwa");
			ListView_InsertColumn(ui_keyList, 0, &lvc);

			lvc.iSubItem = 1;
			lvc.cx = 60;
			lvc.pszText = TEXT("Zaufany");
			ListView_InsertColumn(ui_keyList, 1, &lvc);

			lvc.iSubItem = 2;
			lvc.cx = 290;
			lvc.pszText = TEXT("Klucz");
			ListView_InsertColumn(ui_keyList, 2, &lvc);

			lvc.iSubItem = 3;
			lvc.cx = 150;
			lvc.pszText = TEXT("Konto");
			ListView_InsertColumn(ui_keyList, 3, &lvc);
		}
		ui_update_keylist();

		// Verify key Button
		// TODO: enable button below (remove WS_DISABLED)
		CreateWindowW(L"BUTTON", L"Werifikuj klucz", WS_CHILD | WS_VISIBLE | WS_DISABLED,
			5, 410, 150, 25, ui_settingsWnd, (HMENU)2, hInstance, NULL);

		// Remove key Button
		CreateWindowW(L"BUTTON", L"Odrzuæ klucz", WS_CHILD | WS_VISIBLE,
			200, 410, 150, 25, ui_settingsWnd, (HMENU)3, hInstance, NULL);


		delete[] buffer;
		delete[] wc_buffer;
	}

	return 0;
}

WTW_PTR SettingsBroker::onActionHide(wtwOptionPageShowInfo* pInfo)
{
	//MessageBox(ui_settingsWnd, L"onActionHide", L"Parametry", MB_OK);
	ShowWindow(ui_settingsWnd, SW_HIDE);
	return 0;
}

WTW_PTR SettingsBroker::onActionOk(wtwOptionPageShowInfo* pInfo)
{
	// Save settings
	onActionApply(pInfo);

	// Destroy window
	if (ui_settingsWnd) {
		DestroyWindow(ui_settingsWnd);
		ui_settingsWnd = 0;
	}

	return 0;
}


WTW_PTR SettingsBroker::onActionApply(wtwOptionPageShowInfo* pInfo)
{
	// Save settings
	if (nullptr != pSettingsInterface)
	{
		GetWindowText(ui_privateKeyFilePath, settingsMap[SETTING_KEY_PRIV_KEY_FILE_PATH].value_wc_string, MAX_FILE_PATH_LEN);
		wtwSetStr(wtw, pSettingsInterface, SETTINGS_WTW_NAMES[SETTING_KEY_PRIV_KEY_FILE_PATH], getPrivateKeyFullPath());
		//wtwSetStr(wtw,pSettingsInterface, SETTINGS_WTW_NAMES[SETTING_KEY_PRIV_KEY_FILE_PATH], L"c:\\golkow\\dwa");
		//MessageBox(ui_settingsWnd, getPrivateKeyFullPath(), L"PrivateKeyFile - to zapisalem", MB_OK);

		// TODO: save accountName & protocol and also other settings members in this class

		if (1 == wtw->fnCall(WTW_SETTINGS_WRITE, reinterpret_cast<WTW_PARAM>(pSettingsInterface), NULL)) {
			//LOG_DEBUG(L"Settings saved successfully");
		}
		else {
			LOG_ERROR(L"Saving settings to file failed!");
			MessageBox(ui_settingsWnd, L"Saving settings to file failed!", L"wtwOTRmessaging error", MB_OK);
			return -1;
		}
	}
	else {
		LOG_ERROR(L"SettingsBroker::onActionApply() pSettingsInterface = nullptr");
		return -2;
	}

	return 0;
}


WTW_PTR SettingsBroker::onActionCancel(wtwOptionPageShowInfo* pInfo)
{
	//MessageBox(NULL, L"onActionCancel", L"Parametry", MB_OK);
	if (ui_settingsWnd) {
		DestroyWindow(ui_settingsWnd);
		ui_settingsWnd = 0;
	}
	return 0;
}


WTW_PTR SettingsBroker::onActionMove(wtwOptionPageShowInfo* pInfo)
{
	//MessageBox(NULL, L"onActionMove", L"Parametry", MB_OK);
	//LOG_DEBUG(L"SettingsBroker::onActionMove() not implemented! %d,%d   %d,%d", pInfo->x, pInfo->y, pInfo->cx, pInfo->cy);

	if (nullptr != ui_settingsWnd) {
		SetWindowPos(ui_settingsWnd, HWND_TOP, pInfo->x, pInfo->y, pInfo->cx, pInfo->cy, pInfo->flags);
	} else {
		LOG_ERROR(L"SettingsBroker::onActionMove() - ui_settingsWnd is null!");
	}
	
	return 0;
}


WTW_PTR SettingsBroker::onActionReload(wtwOptionPageShowInfo* pInfo)
{
	//MessageBox(NULL, L"onActionReload", L"Parametry", MB_OK);

	if (nullptr != pSettingsInterface) {
		if (1 == wtw->fnCall(WTW_SETTINGS_READ, reinterpret_cast<WTW_PARAM>(pSettingsInterface), NULL)) {
			//LOG_DEBUG(__FUNCTIONW__ L" Read settings from file successfully");

			wchar_t *buffer;
			wtwGetStr(wtw, pSettingsInterface, SETTINGS_WTW_NAMES[SETTING_KEY_PRIV_KEY_FILE_PATH], L"default-val", &buffer);
			if (L'\0' == buffer[0]) {
				LOG_WARN(__FUNCTIONW__ L" PRIV KEY file name is empty!");
			}
			wcscpy_s(settingsMap[SETTING_KEY_PRIV_KEY_FILE_PATH].value_wc_string,
				sizeof(settingsMap[SETTING_KEY_PRIV_KEY_FILE_PATH].value_wc_string) / sizeof(wchar_t),
				buffer);
			//MessageBox(NULL, s_privateKeyFilePath, L"Zapisano do zmiennej: PrivateKeyFile - wartosc", MB_OK);
			delete buffer;
		}
		else
		{
			LOG_WARN(__FUNCTIONW__ L" Reading settings from file failed - setting default values!");

			CString key_file = getSettingsDataDir();
			key_file += L"key.secret";
			wcscpy_s(settingsMap[SETTING_KEY_PRIV_KEY_FILE_PATH].value_wc_string,
				sizeof(settingsMap[SETTING_KEY_PRIV_KEY_FILE_PATH].value_wc_string) / sizeof(wchar_t),
				key_file);
		}

		
	}
	else {
		LOG_CRITICAL(L"SettingsBroker::onActionReload() No interface to reload settings!");
		return -2;
	}

	return 0;
}



void SettingsBroker::setSettingsBrokerInstancePointer()
{
	if (0 == settingsBrokerInstance) {
		settingsBrokerInstance = this;	// Set instance pointer
		ui_settingsWnd = 0;				// Make sure window pointer is cleared
	} else {
		LOG_CRITICAL(L"Second instance of Settings cannot be created!");
		// TODO: retun some error so that plugin get unloaded
		return;
	}
}


void SettingsBroker::unsetSettingsBrokerInstancePointer()
{
	if (0 != settingsBrokerInstance) {
		settingsBrokerInstance = 0;
	} else {
		LOG_CRITICAL(L"Settings instance does not exist!");
	}
}


void SettingsBroker::makeSettingsFilesPaths()
{
	// Get directory
	wtwDirectoryInfo di;
	initStruct(di);
	di.flags = WTW_DIRECTORY_FLAG_FULLPATH;
	di.dirType = WTW_DIRECTORY_PLUGINDATA;
	di.bi.bufferSize = MAX_FILE_PATH_LEN;
	di.bi.pBuffer = settingsMap[SETTING_KEY_DATA_DIR].value_wc_string;

	if (S_OK == wtw->fnCall(WTW_GET_DIRECTORY_LOCATION, reinterpret_cast<WTW_PARAM>(&di), NULL)) {
#if 0
		wcscpy_s(s_settingsFilePath, MAX_FILE_PATH_LEN, settingsMap[SETTING_KEY_DATA_DIR].value_wc_string);
		size_t start = wcslen(settingsMap[SETTING_KEY_DATA_DIR].value_wc_string);
		//s_settingsFilePath[start++] = L'\\';
	
		wcscpy_s(&settingsMap[SETTING_KEY_SETTINGS_FILE_PATH].value_wc_string[start],
				MAX_FILE_PATH_LEN - start, SETTINGS_FILE_NAME);
#else
		StringCbPrintfW(settingsMap[SETTING_KEY_SETTINGS_FILE_PATH].value_wc_string,
						sizeof(settingsMap[SETTING_KEY_SETTINGS_FILE_PATH].value_wc_string), L"%s%s",
						settingsMap[SETTING_KEY_DATA_DIR].value_wc_string, SETTINGS_FILE_NAME);
#endif

		//LOG_DEBUG(L"%s() settings file path: %s", __FUNCTIONW__, settingsMap[SETTING_KEY_SETTINGS_FILE_PATH].value_wc_string);

		StringCbPrintfW(settingsMap[SETTING_KEY_INSTAG_FILE_PATH].value_wc_string,
			sizeof(settingsMap[SETTING_KEY_INSTAG_FILE_PATH].value_wc_string), L"%s%s",
			settingsMap[SETTING_KEY_DATA_DIR].value_wc_string, INSTAG_FILE_NAME);

		StringCbPrintfW(settingsMap[SETTING_KEY_FINGERPRINT_FILE_PATH].value_wc_string,
			sizeof(settingsMap[SETTING_KEY_FINGERPRINT_FILE_PATH].value_wc_string), L"%s%s",
			settingsMap[SETTING_KEY_DATA_DIR].value_wc_string, FINGERPRINT_FILE_NAME);
	}
	else {
		LOG_CRITICAL(L"Cannot get plugin data directory!");
		// TODO: make error state in class so that the we can check if all classes initialized properly
		//       if not, the plugin could be unloaded immediately
		return;
	}
}


void SettingsBroker::addSettingsPage()
{
	// Add settings page
	wtwOptionPageDef optionsPage;
	initStruct(optionsPage);
	optionsPage.id = WTWOTR_OPTIONS_PAGE_ID;
	optionsPage.parentId = WTW_OPTIONS_GROUP_PLUGINS;
	optionsPage.caption = L"wtwOTRmessaging";
	//optionsPage.flags = ;
	// TODO: set icon for settings (better place it in resources)
	//optionsPage.icon = ;
	//optionsPage.iconId = ;
	optionsPage.callback = SettingsBroker::settings_callback;
	optionsPage.cbData = this;
	//optionsPage.ownerData = ;
	wtw->fnCall(WTW_OPTION_PAGE_ADD, reinterpret_cast<WTW_PARAM>(hInstance),
		reinterpret_cast<WTW_PARAM>(&optionsPage));
}


void SettingsBroker::removeSettingsPage()
{
	wtw->fnCall(WTW_OPTION_PAGE_REMOVE, reinterpret_cast<WTW_PARAM>(hInstance),
		reinterpret_cast<WTW_PARAM>(WTWOTR_OPTIONS_PAGE_ID));
}


void SettingsBroker::registerWndClass()
{
	WNDCLASSEX wc;
	memset(&wc, 0, sizeof(wc));

	// Register only if not yet registered
	if (0 == GetClassInfoEx(hInstance, SettingsWindowClassName, &wc)) {
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = 0;
		wc.lpfnWndProc = (WNDPROC)SettingsBroker::settings_class_callback;
		//wc.lpfnWndProc = settings_class_WNDPROC; // this cause that background is not repainted well - let live with it now ;)
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		//wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
		wc.hIcon = 0;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = SettingsWindowClassName;
		//wc.hIconSm = LoadIcon( NULL, IDI_APPLICATION );
		wc.hIconSm = 0;

		if (!RegisterClassEx(&wc)) {
			LOG_CRITICAL(L"Registering settings window failed!");
			return;
		}
	}
}


// Initialize settings
void SettingsBroker::initializeSettingsValues()
{
	for (int index = 0; index < SETTING_KEY_NUMBER_OF_KEYS; ++index)
	{
		switch (index)
		{
		case SETTING_KEY_DATA_DIR:
			settingsMap[index].type = SETTING_TYPE_WC_STRING;
			break;
		case SETTING_KEY_SETTINGS_FILE_PATH:
			settingsMap[index].type = SETTING_TYPE_WC_STRING;
			break;
		case SETTING_KEY_PRIV_KEY_FILE_PATH:
			settingsMap[index].type = SETTING_TYPE_WC_STRING;
			break;
		case SETTING_KEY_INSTAG_FILE_PATH:
			settingsMap[index].type = SETTING_TYPE_WC_STRING;
			break;
		case SETTING_KEY_FINGERPRINT_FILE_PATH:
			settingsMap[index].type = SETTING_TYPE_WC_STRING;
			break;
		case SETTING_KEY_PROTOCOL_NET_CLASS:
			settingsMap[index].type = SETTING_TYPE_WC_STRING;
			wcscpy_s(settingsMap[index].value_wc_string,
				sizeof(settingsMap[index].value_wc_string) / sizeof(wchar_t),
				L"XMPP");
			break;
		case SETTING_KEY_PROTOCOL_NET_ID:
			settingsMap[index].type = SETTING_TYPE_INT;
			settingsMap[index].value_int = 1;
			break;
		case SETTING_KEY_OTRL_ACCOUNT_NAME:
			settingsMap[index].type = SETTING_TYPE_C_STRING;
			_snprintf_s(settingsMap[index].value_c_string,
				sizeof(settingsMap[index].value_c_string),
				sizeof(settingsMap[index].value_c_string),
				"account_name");
			break;

		default:
			LOG_CRITICAL(L"SettingsBroker::setSettingsDefaultValues() - no default val for %d", index);
		}
	}
}


void SettingsBroker::createSettingsIterface()
{
	/*
	wchar_t buff[MAX_PATH + 1];
	wtwMyConfigFile configName;
	configName.bufferSize = MAX_PATH;
	configName.pBuffer = buff;
	wtw->fnCall(WTW_SETTINGS_GET_MY_CONFIG_FILE, configName, reinterpret_cast<WTW_PARAM>(hInstance));
	LOG_DEBUG(L"createSettingsIterface() WTW_SETTINGS_GET_MY_CONFIG_FILE: '%s'", configName.pBuffer);
	LOG_DEBUG(L"createSettingsIterface() MY: '%s'", getSettingsFileFullPath());
	delete [] configName.pBuffer;
	*/

	if (S_OK == wtw->fnCall(WTW_SETTINGS_INIT_EX,
		reinterpret_cast<WTW_PARAM>(getSettingsFileFullPath()),
		reinterpret_cast<WTW_PARAM>(&pSettingsInterface)))
	{
		//LOG_DEBUG(L"SettingsBroker::createSettingsIterface() pSettingsInterface created");
		// Read settings from file
		onActionReload(0);
	}
	else {
		pSettingsInterface = nullptr;
		LOG_CRITICAL(L"Cannot create interface to settings!");
		// TODO: unload plugin & report error
		return;
	}
}


void SettingsBroker::destroySettingsInterface()
{
	wtw->fnCall(WTW_SETTINGS_DESTROY, reinterpret_cast<WTW_PARAM>(pSettingsInterface), NULL);
	pSettingsInterface = 0;
}



void SettingsBroker::ui_update_keylist()
{
	ConnContext * context;
	Fingerprint * fingerprint;
	int index = 0;

	// clear the list
	ListView_DeleteAllItems(ui_keyList);
	keyList_fingerprints.clear();

	for(context = itsWtwOTRmessaging->getOtrlUserState()->context_root;
		context != NULL; context = context->next)
	{
		if (context->m_context != context) // skip child context
			continue;

		fingerprint = context->fingerprint_root.next;
		while (fingerprint) // If there's no fingerprint, don't add it to the known fingerprints list
		{
			char hash[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
			otrl_privkey_hash_to_human(hash, fingerprint->fingerprint);

			LVITEM   lv = { 0 };
			lv.iItem = index;
			ListView_InsertItem(ui_keyList, &lv);
			ListView_SetItemText(ui_keyList, index, 0, (wchar_t*)utf8Toutf16(context->username));
			ListView_SetItemText(ui_keyList, index, 1, (otrl_context_is_fingerprint_trusted(fingerprint)) ? L"TAK" : L"NIE");
			ListView_SetItemText(ui_keyList, index, 2, (wchar_t*)utf8Toutf16(hash));
			ListView_SetItemText(ui_keyList, index, 3, (wchar_t*)utf8Toutf16(context->accountname));
			keyList_fingerprints.push_back(fingerprint);
			++index;

			fingerprint = fingerprint->next;

			//LOG_INFO(L"dodalem '%s', '%s', '%s'", utf8Toutf16(context->username),
			//	(otrl_context_is_fingerprint_trusted(fingerprint)) ? L"TAK" : L"NIE", utf8Toutf16(hash));
		}

		/*		
		LOG_DEBUG(	L"%s()\n"
					L"accountname: '%s'\n"
					L"username: '%s'\n"
					L"protocol: '%s'\n"
					L"0plain/1enc/2fshd %d\n"
					L"authstate: %d\n"
					L"offer (not/sent/rej/acptd) %d\n",
			utf8Toutf16(__FUNCTION__),
			utf8Toutf16(context->accountname),
			utf8Toutf16(context->username),
			utf8Toutf16(context->protocol),
			(int)context->msgstate,
			(int)context->auth.authstate,
			(int)context->otr_offer
			);
		*/
	}
	
	
	
/*	FROM PIDGIN PLUGIN

	gchar *titles[5];
	char hash[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
	ConnContext * context;
	Fingerprint * fingerprint;
	int selected_row = -1;

	GtkWidget *keylist = ui_layout.keylist;

	if (keylist == NULL)
		return;

	gtk_clist_freeze(GTK_CLIST(keylist));
	gtk_clist_clear(GTK_CLIST(keylist));

	for (context = otrg_plugin_userstate->context_root; context != NULL;
		context = context->next) {
		int i;
		PurplePlugin *p;
		char *proto_name;

		if (context->m_context != context) continue;

		fingerprint = context->fingerprint_root.next;
		// If there's no fingerprint, don't add it to the known fingerprints list
		while (fingerprint) {
			ConnContext *context_iter;
			TrustLevel best_level = TRUST_NOT_PRIVATE;
			int used = 0;

			titles[0] = context->username;
			titles[1] = _("Unused");

			for (context_iter = context->m_context;
				context_iter && context_iter->m_context == context->m_context;
				context_iter = context_iter->next) {

				TrustLevel this_level = TRUST_NOT_PRIVATE;

				if (context_iter->active_fingerprint == fingerprint) {
					this_level = otrg_plugin_context_to_trust(context_iter);
					used = 1;

					if (this_level == TRUST_PRIVATE) {
						best_level = TRUST_PRIVATE;
					}
					else if (this_level == TRUST_UNVERIFIED
						&& best_level != TRUST_PRIVATE) {
						best_level = TRUST_UNVERIFIED;
					}
					else if (this_level == TRUST_FINISHED
						&& best_level == TRUST_NOT_PRIVATE) {
						best_level = TRUST_FINISHED;
					}
				}
			}

			if (used) {
				titles[1] = (gchar *)
					_(trust_states[best_level]);
			}
			titles[2] = (fingerprint->trust && fingerprint->trust[0]) ?
				_("Yes") : _("No");
			otrl_privkey_hash_to_human(hash, fingerprint->fingerprint);
			titles[3] = hash;
			p = purple_find_prpl(context->protocol);
			proto_name = (p && p->info->name) ? p->info->name : _("Unknown");
			titles[4] = g_strdup_printf("%s (%s)", context->accountname,
				proto_name);
			i = gtk_clist_append(GTK_CLIST(keylist), titles);
			g_free(titles[4]);
			gtk_clist_set_row_data(GTK_CLIST(keylist), i, fingerprint);
			if (ui_layout.selected_fprint == fingerprint) {
				selected_row = i;
			}
			fingerprint = fingerprint->next;
		}
	}

	if (selected_row >= 0) {
		gtk_clist_select_row(GTK_CLIST(keylist), selected_row, 0);
	}
	else {
		clist_all_unselected();
	}

	gtk_clist_sort(GTK_CLIST(keylist));

	gtk_clist_thaw(GTK_CLIST(keylist));
*/

}


void SettingsBroker::ui_forget_fingerprint()
{
	int i = ListView_GetNextItem(settingsBrokerInstance->ui_keyList, -1, LVNI_SELECTED);
	if (i >= 0)
	{
		ConnContext *context;
		ConnContext *context_iter;
		Fingerprint *fp = static_cast<Fingerprint*>(settingsBrokerInstance->keyList_fingerprints[i]);
		
		if (nullptr == fp)
		{
			LOG_ERROR(__FUNCTIONW__ L"() fp = nullptr");
			return;
		}

		// Don't do anything with the active fingerprint if we're
		// in the ENCRYPTED state.
		context = fp->context;
		for (context_iter = context->m_context;
			context_iter && context_iter->m_context == context->m_context;
			context_iter = context_iter->next)
		{
			if (context_iter->msgstate == OTRL_MSGSTATE_ENCRYPTED &&
				context_iter->active_fingerprint == fp)
			{
				MessageBox(settingsBrokerInstance->ui_settingsWnd,
					#ifdef POLISH
						L"Nie mo¿na usun¹æ odcisku (trwa szyfrowana rozmowa)",
					#else
						L"Cannot forget fingerprint (ENCRYPTED chat)",
					#endif
					L"Info", MB_ICONINFORMATION);
				return;
			}
		}

		char hash[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
		otrl_privkey_hash_to_human(hash, fp->fingerprint);
		CStringW text =
			#ifdef POLISH
				L"Odcisk zosta³ usuniêty\nNazwa: ";
			#else
				L"Fingerprint has been removed\nName: ";
			#endif
		text += utf8Toutf16(fp->context->username);
		text +=
			#ifdef POLISH
				L"\nKlucz: ";
			#else
				L"\nKey: ";
			#endif
		text += utf8Toutf16(hash);

		otrl_context_forget_fingerprint(fp, 1);
		wtwOTRmessaging::OTRL_write_fingerprints_cb(nullptr);
		wtwOTRmessaging::OTRL_update_context_list_cb(nullptr);

		MessageBox(settingsBrokerInstance->ui_settingsWnd, text, L"Info", MB_ICONINFORMATION);
	}
}
