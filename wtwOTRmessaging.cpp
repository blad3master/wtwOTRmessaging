// wtwOTRmessaging.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "wtwAPI/wtwCEL.h"
#include "wtwapi/libWTW-src/include/libWTW/String.h"

#include "wtwOTRmessaging.h"
#include "Logger.h"
#include "utils.h"
#include "Resource.h" // version information
#include "PluginSettings.h"
#include "PeerPolicy.h"
#include <Strsafe.h> // StringCchPrintW()

#include <clocale> // std::set_locale()
#include <cstdlib> // std::wctombs()
#include <codecvt> // std::codecvt

#include <locale>

#include <vector>
#include <algorithm>

extern "C" {
	//#include "libotr/src/message.h"
	#include "OTRL/privkey.h"
}


WtwOtrContext WtwOtrContext::_instances[WtwOtrContext::INSTANCES_TOTAL];

static wtwOTRmessaging * p_wtwOTRmessaging = nullptr;

wtwOTRmessaging *wtwOTRmessaging::instance = nullptr;
//UINT_PTR wtwOTRmessaging::OTRL_timer_id = 0;

// this variable helps to see distinguish WTW_exit from just plugin unload (from options)
DWORD pluginUnload_callReason = 0;

std::vector<CString> wtwOTRmessaging::supportedNetClasses = { L"XMPP", L"GG" };

const OtrlMessageAppOps wtwOTRmessaging::itsOTRLops = {
	wtwOTRmessaging::OTRL_policy_cb,
	wtwOTRmessaging::OTRL_create_privkey_cb,
	wtwOTRmessaging::OTRL_is_logged_in_cb,
	wtwOTRmessaging::OTRL_inject_message_cb,
	wtwOTRmessaging::OTRL_update_context_list_cb,
	wtwOTRmessaging::OTRL_new_fingerprint_cb,
	wtwOTRmessaging::OTRL_write_fingerprints_cb,
	wtwOTRmessaging::OTRL_gone_secure_cb,
	nullptr,		// gone_insecure - not called by OTRL!
	wtwOTRmessaging::OTRL_still_secure_cb,
	wtwOTRmessaging::OTRL_max_message_size_cb,
	NULL,			// account_name - not called by OTRL!
	NULL,           // account_name_free - not called by OTRL!
	NULL,			// received_symkey
	wtwOTRmessaging::OTRL_otr_error_message_cb,
	wtwOTRmessaging::OTRL_otr_error_message_free_cb,
	wtwOTRmessaging::OTRL_resent_msg_prefix_cb,
	wtwOTRmessaging::OTRL_resent_msg_prefix_free_cb,
	wtwOTRmessaging::OTRL_handle_smp_event_cb,
	wtwOTRmessaging::OTRL_handle_msg_event_cb,
	wtwOTRmessaging::OTRL_create_instag_cb,
	NULL,		    // convert_msg
	NULL,		    // convert_free
	wtwOTRmessaging::OTRL_timer_control_cb
};

WTWPLUGINFO plugInfo = {
	sizeof(WTWPLUGINFO),										// rozmiar struktury
	L"wtwOTRmessaging",											// nazwa wtyczki
	L"Implementacja protoko�u Off-the-Record Messaging, "
	L"kt�ry umo�liwia szyfrowanie oraz uwierzytelnianie rozm�w.", // opis wtyczki
	L"(c) 2014-2016",											// copyrights
	L"blad3master",												// autor
	L"blad3master@blad3master.com",								// email
	L"http://blad3master.com",									// strona www
	L"http://blad3master.com/download/wtwOTRmessaging.xml",		// link do pliku AU, podac 0 jezeli nie uzywamy AU
	PLUGIN_API_VERSION,											// wersja api z ktora zostal skompilowany plug
	MAKE_QWORD(RELEASE_VER_A_MAJOR, RELEASE_VER_B_MINOR,
				RELEASE_VER_C_BUILD, RELEASE_VER_D_REVISION),	// wersja plug'a
	WTW_CLASS_UTILITY,											// klasa plugin'a
	0,															// fcja ktora wtw wywola jezeli user bedzie chcial zobaczyc about plug'a
	L"{2195C31D-89D8-41bb-A390-622E88DB476B}",					// GUID, wymagane...
	0,															// zaleznosci (tablica GUID'ow, ostatnia pozycja MA byc 0)
	0, 0, 0, 0													// zarezerwowane
};


WTW_PLUGIN_API_ENTRY(WTWPLUGINFO*) queryPlugInfo(DWORD /*apiVersion*/, DWORD /*masterVersion*/)
{
	return &plugInfo;
}


WTW_PLUGIN_API_ENTRY(int) pluginLoad(DWORD /*callReason*/, WTWFUNCTIONS* f)
{
	std::setlocale(LC_ALL, "en_US.UTF-16");

	// Get handle to instance of the WTW program
	hInstance = (HINSTANCE)GetModuleHandle(NULL);

	wtwPf = f;
	p_wtwOTRmessaging = new wtwOTRmessaging();

	//MessageBox(NULL, L"Wtyczka za�adowana raz", L"Hello world!", MB_OK);
	return 0;
}


WTW_PLUGIN_API_ENTRY(int) pluginUnload(DWORD callReason)
{
	pluginUnload_callReason = callReason;
	if (nullptr != p_wtwOTRmessaging) {
		delete p_wtwOTRmessaging;
		p_wtwOTRmessaging = nullptr;
	}

	wtwPf = 0;
	hInstance = NULL;

	//MessageBox(NULL, L"Wtyczka wy�adowana", L"Hello world!", MB_OK);
	return 0;
}


#pragma warning(disable: 4355) /* 'this' : used in base member initializer list */
wtwOTRmessaging::wtwOTRmessaging() :
	itsSettingsBroker(this)
#pragma warning(default: 4355)
{
	PluginSettings::init();
	PeerPolicy::init(this);
	setInstancePointer();
	initializeOTRL();
	installWtwHooks();
	installWtwFunctions();

	//LOG_DEBUG(L"wtwOTRmessaging::wtwOTRmessaging() completed successfully");
}


wtwOTRmessaging::~wtwOTRmessaging()
{
	// we must remove timer from WTW - if not a crash would happen at next TICK :P
	stop_start_timer(0);

	removeWtwFunctions();
	removeWtwHooks();
	releaseOTRL();
	unsetInstancePointer();
	PluginSettings::destroy();
}


OtrlPolicy wtwOTRmessaging::OTRL_policy_cb(void *opdata, ConnContext *context)
{
	//LOG_DEBUG(L"%s()\naccountname:'%s'\nprotocol : '%s'\nusername : '%s'\n", __FUNCTIONW__,
	//	utf8Toutf16(context->accountname), utf8Toutf16(context->protocol), utf8Toutf16(context->username));

	//TODO: return policy set in gui(settings) and return it instead
	// of default value

	OtrlPolicy policy = OTRL_POLICY_DEFAULT;

#if 0
	switch (instance->getSettingsBroker().getOtrlPolicy()) FIX
	{
	case SettingsBroker::OTRL_POLICY::NEVER:			policy = OTRL_POLICY_NEVER;			break;
	case SettingsBroker::OTRL_POLICY::MANUAL:			policy = OTRL_POLICY_MANUAL;		break;
	case SettingsBroker::OTRL_POLICY::OPPORTUNISTIC:	policy = OTRL_POLICY_OPPORTUNISTIC;	break;
	case SettingsBroker::OTRL_POLICY::ALWAYS:			policy = OTRL_POLICY_ALWAYS;		break;
	default: LOG_CRITICAL(L"%s() wrong policy", __FUNCTIONW__);
	}

	LOG_TRACE(L"%s() selected policy %d, final policy %d", __FUNCTIONW__,
		instance->getSettingsBroker().getOtrlPolicy(), policy); FIX
#else
	
	policy = PeerPolicy::get(utf8Toutf16(context->username),
		utf8Toutf16(context->protocol), WtwOtrContext::netIdFromAccountname(context->accountname));

	const wchar_t *p_text = L"unknown";
	switch (policy) {
	case OTRL_POLICY_NEVER:			p_text = L"NEVER";			break;
	case OTRL_POLICY_MANUAL:		p_text = L"MANUAL";			break;
	case OTRL_POLICY_OPPORTUNISTIC:	p_text = L"OPPORTUNISTIC";	break;
	case OTRL_POLICY_ALWAYS:		p_text = L"ALWAYS";			break;
	}
	std::wstring contact_key = makeKeyFromPeer(utf8Toutf16(context->username),
		utf8Toutf16(context->protocol),	WtwOtrContext::netIdFromAccountname(context->accountname));
	LOG_TRACE(L"%s() policy for user '%s' => '%s'", __FUNCTIONW__, contact_key.c_str(), p_text);

#endif

	return policy;
}


void wtwOTRmessaging::OTRL_create_privkey_cb(void *opdata, const char *accountname, const char *protocol)
{
	LOG_INFO(L"%s() accountname:'%s', protocol:'%s'", __FUNCTIONW__,
		utf8Toutf16(accountname).operator LPCWSTR(), utf8Toutf16(protocol).operator LPCWSTR());

	OtrlPrivKey * key = 0;

	if (0 != otrl_privkey_find(instance->getOtrlUserState(), accountname, protocol))
	{
		LOG_WARN(L"%s() Private key already exists", __FUNCTIONW__);
		MessageBox(nullptr, L"Private key for the user already exists", L"wtwOTRmessaging", MB_ICONINFORMATION);
		return;
	}

	std::string pk_file_path = utf16Toutf8(instance->itsSettingsBroker.getPrivateKeyFullPath());
	if (pk_file_path.empty())
	{
		LOG_CRITICAL(L"Private key file is empty (file '%s')",
			instance->itsSettingsBroker.getPrivateKeyFullPath());
		return;
	}


	otrl_privkey_generate_start(instance->getOtrlUserState(),
					accountname, protocol, reinterpret_cast<void **>(&key));
	if (nullptr != key)
	{
		otrl_privkey_generate_calculate(key);
		//gcry_error_t;
		if (GPG_ERR_NO_ERROR == otrl_privkey_generate_finish(instance->getOtrlUserState(), key, pk_file_path.c_str()))
		{
			LOG_INFO(L"Private key generated successfully (saved to '%s')",
				instance->itsSettingsBroker.getPrivateKeyFullPath());
		}
		else
		{
			LOG_ERROR(L"Private key generated successfully but not saved properly (file '%s')",
				instance->itsSettingsBroker.getPrivateKeyFullPath());
			LOG_CRITICAL(L"Private key generated successfully but not saved properly (file '%s')",
				instance->itsSettingsBroker.getPrivateKeyFullPath());
		}

		// Do not "delete key;" since it is stored in OTRLUserState - delete causes crash
	}
	else
	{
		LOG_CRITICAL(L"Error occurred while generating private key");
		//MessageBox(ui_settingsWnd, L"Error occurred while generating private key", L"wtwOTRmessaging error", MB_ICONINFORMATION);
	}
}


int wtwOTRmessaging::OTRL_is_logged_in_cb(void *opdata, const char *accountname, const char *protocol, const char *recipient)
{
	LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);
	return 1;
}


void wtwOTRmessaging::OTRL_inject_message_cb(void *opdata, const char *accountname, const char *protocol,
	const char *recipient, const char *message)
{
	//LOG_DEBUG(L"SENDING RAW MSG to contact:");
	//LOG_DEBUG(L"%s", utf8Toutf16(message));
	//LOG_DEBUG(L"----------------------------------------------------");

	std::wstring id;
	std::wstring netClass;
	int netId;

	if (opdata)
	{
		WtwOtrContext *wtwOtrContext = reinterpret_cast<WtwOtrContext*>(opdata);
		ASSERT(nullptr != wtwOtrContext->wtw.id);
		ASSERT(nullptr != wtwOtrContext->wtw.netClass);
		ASSERT(0 == wcsncmp(wtwOtrContext->wtw.id, utf8Toutf16(recipient), _countof(wtwOtrContext->wtw.id)));
		ASSERT(0 == wcsncmp(wtwOtrContext->wtw.netClass, utf8Toutf16(protocol), _countof(wtwOtrContext->wtw.netClass)));
		ASSERT(atoi(accountname) == wtwOtrContext->wtw.netId);

		id = wtwOtrContext->wtw.id;
		netClass = wtwOtrContext->wtw.netClass;
		netId = wtwOtrContext->wtw.netId;
	}
	else
	{
		LOG_TRACE(L"%s() opdata is nullptr - fallback to context data", __FUNCTIONW__);
		id = utf8Toutf16(recipient);
		netClass = utf8Toutf16(protocol);
		netId = WtwOtrContext::netIdFromAccountname(accountname);
	}

	sendRawMessageToContact(id.c_str(), netClass.c_str(), netId, utf8Toutf16(message),
		WTW_MESSAGE_FLAG_OUTGOING | WTW_MESSAGE_FLAG_CHAT_MSG | WTW_MESSAGE_FLAG_CEL
		/* | WTW_MESSAGE_FLAG_ENCRYPTED*/);
}


void wtwOTRmessaging::OTRL_update_context_list_cb(void *opdata)
{
	//Logger::error(L"FIXME: OTRL_update_context_list_cb **************");
	//my clues: Probably when list of (known/peers?) key changes we need to update UI fingerprints lists

	// a change of connection might have changed so update the gui controls to reflect the
	// connection status, figerprints, and so on

	// update settings key list
	instance->itsSettingsBroker.ui_update_keylist();

	// update button in chat window
	ChatBroker::update_ui(nullptr);
}


void wtwOTRmessaging::OTRL_new_fingerprint_cb(void *opdata, OtrlUserState us,
	const char *accountname, const char *protocol, const char *username, unsigned char fingerprint[20])
{
	wchar_t s[500];
	char fp_string[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];

	otrl_privkey_hash_to_human(fp_string, fingerprint);
	
	StringCbPrintfW(s, sizeof(s), L"(AccountName: %s Protocol: %s) Otrzymano nowy klucz od u�ytkownika '%s', odcisk '%s'",
		utf8Toutf16(accountname).operator LPCWSTR(), utf8Toutf16(protocol).operator LPCWSTR(),
		utf8Toutf16(username).operator LPCWSTR(), utf8Toutf16(fp_string).operator LPCWSTR());

	instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), s, WTW_MESSAGE_FLAG_INFO);

	// update settings key list
	instance->itsSettingsBroker.ui_update_keylist();

/*
	wtwProtocolEvent ev;
	initStruct(ev);
	ev.netClass = netClass;
	ev.netId = netId;
	ev.event = WTW_PEV_MESSAGE_RECV; //WTW_PEV_RAW_DATA_RECV;
	ev.type = WTW_PEV_TYPE_BEFORE;

	wtwMessageDef md;
	initStruct(md);
	md.contactData.

		WTW_CHATWND_SHOW_MESSAGE

	/ *
	wtwRawDataDef rd;
	initStruct(rd);
	rd.pData = data;
	rd.pDataLen = len;
	rd.flags = flags;
	* /

	
	if (fp->fnCall(WTW_PF_CALL_HOOKS, reinterpret_cast<WTW_PARAM>(&ev), reinterpret_cast<WTW_PARAM>(&rd)) == 0)
	{
		ev.type = WTW_PEV_TYPE_AFTER;
		fp->fnCall(WTW_PF_CALL_HOOKS, reinterpret_cast<WTW_PARAM>(&ev), reinterpret_cast<WTW_PARAM>(&rd));
		return true;
	}
	return false;

*/
}


void wtwOTRmessaging::OTRL_write_fingerprints_cb(void *opdata)
{
	if (GPG_ERR_NO_ERROR == otrl_privkey_write_fingerprints(instance->getOtrlUserState(),
								utf16Toutf8(instance->itsSettingsBroker.getFigerprintFileFullPath())))
	{
		//LOG_DEBUG(L"Fingerprints has been written to file '%s'", instance->itsSettingsBroker.getFigerprintFileFullPath());
	}
	else
	{
		LOG_ERROR(L"Fingerprints has NOT been written to file '%s'", instance->itsSettingsBroker.getFigerprintFileFullPath());
	}
}


/* Call this when a context transitions to ENCRYPTED. */
void wtwOTRmessaging::OTRL_gone_secure_cb(void *opdata,	ConnContext *context)
{
	wchar_t s[500];
	StringCbPrintfW(s, sizeof(s), L"Rozpocz�to prywatn� rozmow� z u�ytkownikiem '%s'",
		utf8Toutf16(context->username).operator LPCWSTR());

	instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), s, WTW_MESSAGE_FLAG_INFO);

	const std::wstring username(utf8Toutf16(context->username));
	const std::wstring protocol(utf8Toutf16(context->protocol));
	wtwContactDef ct;
	ct.id = username.c_str();
	ct.netClass = protocol.c_str();
	ct.netId = WtwOtrContext::netIdFromAccountname(context->accountname);

	// open window so that button gets refreshed
	wtwPf->fnCall(WTW_FUNCT_OPEN_CHAT_WINDOW, reinterpret_cast<WTW_PARAM>(&ct), 0);

	ChatBroker::update_ui(nullptr);

	

/*	Implementation from PIDGIN plugin

		PurpleConversation *conv;
		char *buf;
		char *format_buf;
		TrustLevel level;
		OtrgUiPrefs prefs;
		gboolean * is_multi_inst;

		conv = otrg_plugin_context_to_conv(context, TRUE);
		level = otrg_plugin_context_to_trust(context);

		otrg_ui_get_prefs(&prefs, purple_conversation_get_account(conv),
			context->username);
		if (prefs.avoid_logging_otr) {
			purple_conversation_set_logging(conv, FALSE);
		}

		switch (level) {
		case TRUST_PRIVATE:
			format_buf = g_strdup(
				_("Private conversation with %s started.%s%s"));
			break;

		case TRUST_UNVERIFIED:
			format_buf = g_strdup_printf(_("<a href=\"%s%s\">Unverified</a> "
				"conversation with %%s started.%%s%%s"),
				UNVERIFIED_HELPURL, _("?lang=en"));
			break;

		default:
			// This last case should never happen, since we know
			// we're in ENCRYPTED.
			format_buf = g_strdup(_("Not private conversation with %s "
				"started.%s%s"));
			break;
		}
		buf = g_strdup_printf(format_buf,
			purple_conversation_get_name(conv),
			context->protocol_version == 1 ? _("  Warning: using old "
			"protocol version 1.") : "", conv->logging ?
			_("  Your client is logging this conversation.") :
			_("  Your client is not logging this conversation."));

		purple_conversation_write(conv, NULL, buf, PURPLE_MESSAGE_SYSTEM,
			time(NULL));

		g_free(buf);
		g_free(format_buf);

		dialog_update_label(context);

		is_multi_inst = (gboolean *)purple_conversation_get_data(conv,
			"otr-conv_multi_instances");

		if (*is_multi_inst) {
			gboolean * have_warned_instances = (gboolean *)
				purple_conversation_get_data(conv, "otr-warned_instances");

			if (!*have_warned_instances) {
				*have_warned_instances = TRUE;
				buf = g_strdup_printf(_("Your buddy is logged in multiple times and"
					" OTR has established <a href=\"%s%s\">multiple sessions"
					"</a>. Use the icon menu above if you wish to select the "
					"outgoing session."), SESSIONS_HELPURL, _("?lang=en"));
				otrg_gtk_dialog_display_otr_message(context->accountname,
					context->protocol, context->username, buf, 0);
				g_free(buf);
			}
		}
*/

}


void wtwOTRmessaging::OTRL_still_secure_cb(void *opdata, ConnContext *context, int is_reply)
{
	wchar_t s[500];
	StringCbPrintfW(s, sizeof(s), L"Pomy�lnie od�wie�ono prywatn� rozmow� z u�ytkownikiem '%s'",
		utf8Toutf16(context->username).operator LPCWSTR());
	wtwOTRmessaging::instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), s, WTW_MESSAGE_FLAG_INFO);
}


int wtwOTRmessaging::OTRL_max_message_size_cb(void *opdata, ConnContext *context)
{
	LOG_TRACE(L"%s() FIXME - now returns hardcoded value", __FUNCTIONW__);


	/* GG: http://libgadu.net/protocol/#ch1.6
	 * D�ugo�� tre�ci wiadomo�ci nie powinna przekracza� 2000 znak�w. 
	 */
	
	// TODO: create an array of known protocols->MaxMessageSize mappings
	return 1000;
}



void wtwOTRmessaging::OTRL_received_symkey_cb(void *opdata,	ConnContext *context, unsigned int use,
						const unsigned char *usedata, size_t usedatalen, const unsigned char *symkey)
{
	LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);
}


const char *wtwOTRmessaging::OTRL_otr_error_message_cb(void *opdata, ConnContext *context, OtrlErrorCode err_code)
{
	LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);
	return 0;
}


void wtwOTRmessaging::OTRL_otr_error_message_free_cb(void *opdata, const char *err_msg)
{
	LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);
}


const char *wtwOTRmessaging::OTRL_resent_msg_prefix_cb(void *opdata, ConnContext *context)
{
	LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);
	return 0;
}


void wtwOTRmessaging::OTRL_resent_msg_prefix_free_cb(void *opdata, const char *prefix)
{
	LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);
}


void wtwOTRmessaging::OTRL_handle_smp_event_cb(void *opdata, OtrlSMPEvent smp_event, ConnContext *context,
												unsigned short progress_percent, char *question)
{
	LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);
}


void wtwOTRmessaging::OTRL_handle_msg_event_cb(void *opdata, OtrlMessageEvent msg_event, ConnContext *context,
												const char *message, gcry_error_t err)
{
	if (nullptr == context)
	{
		LOG_CRITICAL(L"%s() context is nullptr for event %d", __FUNCTIONW__, (int)msg_event);
		return;
	}

	wchar_t msg[2048];
	const std::wstring peer_name = utf8Toutf16(context->username);
	WtwOtrContext *wtwOtrContext = reinterpret_cast<WtwOtrContext*>(opdata);

	if (nullptr == wtwOtrContext)
	{
		LOG_CRITICAL(L"%s() opdata is nullptr for event %d", __FUNCTIONW__, (int)msg_event);
		return;
	}

	switch (msg_event)
	{
	case OTRL_MSGEVENT_NONE:
		LOG_TRACE(L"%s() OTRL_MSGEVENT_NONE", __FUNCTIONW__);
		break;
	case OTRL_MSGEVENT_ENCRYPTION_REQUIRED:
		StringCbPrintfW(msg, sizeof(msg), L"Pr�bowa�e� wys�a� niezaszyfrowan� wiadomo�� do u�ytkownika '%s'. "
			L"Poniewa� wybrana polityka szyfrowania wymaga szyfrowania, zostanie podj�ta pr�ba nawi�zania "
			L"bezpiecznego po��czenia (kiedy to si� stanie Twoja wiadomo�� zostanie wys�ana).", peer_name.c_str());
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_INFO);
		break;
	case OTRL_MSGEVENT_ENCRYPTION_ERROR:
		StringCbPrintfW(msg, sizeof(msg), L"Podczas szyfrowania Twojej wiadomo�ci wyst�pi� b��d. "
			L"Wiadomo�� nie zosta�a wys�ana.");
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_ERROR);
		break;
	case OTRL_MSGEVENT_CONNECTION_ENDED:
	{
		WtwOtrContext *wtwOtrContext = reinterpret_cast<WtwOtrContext*>(opdata);
		if (wtwOtrContext)
		{
			wtwOtrContext->otr.privateConverstationEnded = true; // do not let an unencrypted message out

			StringCbPrintfW(msg, sizeof(msg),
				L"Wiadomo�� do u�ytkownika '%s' NIE zosta�a wys�ana poniewa� "
				L"osoba z kt�r� rozmawiasz zako�czy�a prywatne po��czenie. "
				L"Zako�cz prywatn� rozmow� lub ponowni� j� rozpocznij.",
				peer_name.c_str());
			instance->displayMsgInChat(wtwOtrContext, msg, WTW_MESSAGE_FLAG_INFO);

			const std::wstring netClass(utf8Toutf16(context->protocol));
			wtwContactDef ct;
			ct.id = peer_name.c_str();
			ct.netClass = netClass.c_str();
			ct.netId = WtwOtrContext::netIdFromAccountname(context->accountname);
			ChatBroker::update_ui(nullptr);
		}
		else {
			LOG_CRITICAL(L"%s() opdata is nullptr", __FUNCTIONW__);
		}
		break;
	}
	case OTRL_MSGEVENT_SETUP_ERROR:
		if (!err) {
			err = GPG_ERR_INV_VALUE;
		}
		switch (gcry_err_code(err)) {
		case GPG_ERR_INV_VALUE:
			StringCbPrintfW(msg, sizeof(msg), L"Wyst�pi� b��d podczas pr�by nawi�zania prywatnego po��czenia "
				L"(otrzymano niepoprawn� wiadomo��).");
			break;
		default:
			StringCbPrintfW(msg, sizeof(msg), L"Wyst�pi� b��d podczas pr�by nawi�zania prywatnego po��czenia "
				L"(%s).", utf8Toutf16(gcry_strerror(err)).operator LPCWSTR());
			break;
		}
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_ERROR);
		break;
	case OTRL_MSGEVENT_MSG_REFLECTED:
		StringCbPrintfW(msg, sizeof(msg), L"Otrzymujesz spowrotem swoje wiadomo�ci OTR. Albo rozmawiasz sam "
			L"ze sob�, albo kto� odsy�a Twoje wiadomo�ci do Ciebie!");
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_WARNING);
		break;
	case OTRL_MSGEVENT_MSG_RESENT:
		StringCbPrintfW(msg, sizeof(msg), L"Ostatnia wiadomo�� wys�ana do '%s' zosta�a zretransmitowana.",
			peer_name.c_str());
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_INFO);
		break;
	case OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE:
		StringCbPrintfW(msg, sizeof(msg),
			L"Otrzymano zaszyfrowan� wiadomo�� od u�ytkownika %s, jednak nie mo�na jej odczyta� "
			L"poniewa� <u>nie prowadzisz ju� prywatnej</u> rozmowy. Je�li zamiezasz kontynuowa� rozmow� "
			L"prosz� ponownie <u>rozpocz�� prywatn� rozmow�</u>.", peer_name.c_str());
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_ERROR);
		break;
	case OTRL_MSGEVENT_RCVDMSG_UNREADABLE:
		StringCbPrintfW(msg, sizeof(msg),
			L"Otrzymano wiadomo�� od u�ytkownika %s, kt�rej nie mo�na odczyta�.", peer_name.c_str());
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_ERROR);
		break;
	case OTRL_MSGEVENT_RCVDMSG_MALFORMED:
		StringCbPrintfW(msg, sizeof(msg),
			L"Otrzymano uszkodzon� wiadomo�� od u�ytkownika %s.", peer_name.c_str());
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_ERROR);
		break;
	case OTRL_MSGEVENT_LOG_HEARTBEAT_RCVD:
		// Do not print information about hearbit since it is "detected"
		// when the peer finishes private conversation - the message might
		// be confusing for the user :)
		LOG_TRACE(L"%s() heartbeat recevied from %s/%s/%d", __FUNCTIONW__,
			wtwOtrContext->wtw.id, wtwOtrContext->wtw.netClass, wtwOtrContext->wtw.netId);
		/*
		StringCbPrintfW(msg, sizeof(msg),
			L"Otrzymano heartbeat od u�ytkownika '%s'",
			peer_name);
		instance->displayMsgInChat(peer_name, msg);
		*/
		break;
	case OTRL_MSGEVENT_LOG_HEARTBEAT_SENT:
		LOG_TRACE(L"%s() heartbeat sent to %s/%s/%d", __FUNCTIONW__,
			wtwOtrContext->wtw.id, wtwOtrContext->wtw.netClass, wtwOtrContext->wtw.netId);
		break;
	case OTRL_MSGEVENT_RCVDMSG_GENERAL_ERR:
		StringCbPrintfW(msg, sizeof(msg), L"Wyst�pi� b��d (%s).", utf8Toutf16(message).operator LPCWSTR());
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_ERROR);
		break;
	case OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED:
		StringCbPrintfW(msg, sizeof(msg),
			L"<b>Wiadomo�� otrzymana od u�ytkownika '%s' <i>nie</i> by�a zaszyfrowana: [</b>%s<b>]</b>",
			peer_name.c_str(), utf8Toutf16(message).operator LPCWSTR());
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_WARNING, false, true, true);
		break;
	case OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED:
		LOG_CRITICAL(L"%s() Otrzymano nieznan� wiadomo�� OTR od u�ytkownika %s", __FUNCTIONW__, peer_name.c_str());
		break;
	case OTRL_MSGEVENT_RCVDMSG_FOR_OTHER_INSTANCE:
		//if (*last_msg_event == msg_event) {
		//	break;
		//}
		StringCbPrintfW(msg, sizeof(msg), L"%s wys�a� wiadomo��, kt�ra by�a adresowana do innej sesji. Je�li jeste� "
			L"zalogowany z wielu urzadze�, wiadomo�� ta mog�a trafi� na jedno z nich.", peer_name.c_str());
		instance->displayMsgInChat(reinterpret_cast<WtwOtrContext*>(opdata), msg, WTW_MESSAGE_FLAG_INFO, false, true, true);
		break;
	default:
		LOG_WARN(L"%s() - handling of event %d is not yet implemented", __FUNCTIONW__, (int)msg_event);
	}
}


void wtwOTRmessaging::OTRL_create_instag_cb(void *opdata, const char *accountname, const char *protocol)
{
	FILE *instagf;

	const std::string instagfile(utf16Toutf8(instance->itsSettingsBroker.getInstagFileFullPath()));
	if (instagfile.empty()) {
		LOG_ERROR(L"Instag file path is empty");
		return;
	}

	LOG_DEBUG(L"Creating instag & save to file %s", utf8Toutf16(instagfile.c_str()).operator LPCWSTR());

	if (0 == fopen_s(&instagf, instagfile.c_str(), "w+b"))
	{
		/* Generate the instag */
		otrl_instag_generate_FILEp(instance->itsOtrlUserState, instagf, accountname, protocol);
		fclose(instagf);
	}
	else
	{
		LOG_ERROR(L"%s() Could not write private key file\n", __FUNCTIONW__);
	}
}


void wtwOTRmessaging::OTRL_convert_msg_cb(void *opdata, ConnContext *context, OtrlConvertType convert_type,
											char ** dest, const char *src)
{
	LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);
}


void wtwOTRmessaging::OTRL_convert_free_cb(void *opdata, ConnContext *context, char *dest)
{
	LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);
}


void wtwOTRmessaging::OTRL_timer_control_cb(void *opdata, unsigned int interval)
{
	instance->stop_start_timer(interval);
}


void wtwOTRmessaging::otrl_ConnContex_created(void *data, ConnContext *context)
{
	// For now, the information that a new context has been created during fingerprint read is not exploited

	LOG_TRACE(L"%s() username '%s', protocol '%s', accountname '%s'", __FUNCTIONW__,
		utf8Toutf16(context->username).operator LPCWSTR(),
		utf8Toutf16(context->protocol).operator LPCWSTR(),
		utf8Toutf16(context->accountname).operator LPCWSTR());

	if (nullptr == context->app_data)
	{
		WtwOtrContext *wtwotrc = WtwOtrContext::getInstance();
		if (nullptr != wtwotrc)
		{
			context->app_data = wtwotrc;
			context->app_data_free = WtwOtrContext::releaseInstance;
			wtwotrc->otr.context = context;

			strncpy_s(wtwotrc->otr.username, sizeof(wtwotrc->otr.username),
				context->username, sizeof(wtwotrc->otr.username));
			strncpy_s(wtwotrc->otr.accountname, sizeof(wtwotrc->otr.accountname),
				context->accountname, sizeof(wtwotrc->otr.accountname));
			strncpy_s(wtwotrc->otr.protocol, sizeof(wtwotrc->otr.protocol),
				context->protocol, sizeof(wtwotrc->otr.protocol));

			StringCbPrintfW(wtwotrc->wtw.id, sizeof(wtwotrc->wtw.id), L"%s",
				utf8Toutf16(wtwotrc->otr.username).operator LPCWSTR());
			StringCbPrintfW(wtwotrc->wtw.netClass, sizeof(wtwotrc->wtw.netClass), L"%s",
				utf8Toutf16(wtwotrc->otr.protocol).operator LPCWSTR());
			wtwotrc->wtw.netId = WtwOtrContext::netIdFromAccountname(wtwotrc->otr.accountname);
		}
	}
	else
	{
		LOG_CRITICAL(L"%s() context->app_data is already set!", __FUNCTIONW__);
	}
}



/* Stop the timer, if it's currently running.  If interval > 0, start it
* to periodically fire every interval seconds. */
void wtwOTRmessaging::stop_start_timer(unsigned int interval)
{
#if USE_WINAPI_TIMERS
	if (otrl_timer_set)
	{
		if (0 == KillTimer(NULL, otrl_timer_id)) {
			LOG_ERROR(L"%s() KillTimer failed (timer id %llu)!", __FUNCTIONW__, otrl_timer_id);
		}
		otrl_timer_set = false;
	}

	if (interval > 0)
	{
		unsigned period = (interval <= 11) ? interval * 1000 : 11000;
		otrl_timer_id = SetTimer(NULL, 0, period, otrl_timer_callback);
		if (0 != otrl_timer_id) {
			otrl_timer_set = true;
		} else {
			LOG_ERROR(L"%s() failed to start timer with interval %ums", __FUNCTIONW__, period);
		}
	}
#else
	static const wchar_t * id = L"otrl_timer_id";

	if (otrl_timer_set)
	{
		wtwPf->fnCall(WTW_TIMER_DESTROY, TO_WTW_PARAM(id), TO_WTW_PARAM(nullptr));
		otrl_timer_set = false;
	}

	if (interval > 0)
	{
		wtwTimerDef timerDef;
		timerDef.id = id;
		timerDef.sleepTime = (interval*1000 > 15000) ? 15000 : interval*1000; // sleepTime in ms
		timerDef.callback = otrl_timer_callback;
		//timerDef.flags = WTW_TIMER_FLAG_ONE_TICK; // single-shot timer - we want periodic

		if (1 == wtwPf->fnCall(WTW_TIMER_CREATE, TO_WTW_PARAM(&timerDef), TO_WTW_PARAM(nullptr))) {
			otrl_timer_set = true;
			//LOG_INFO(L"%s() timer created with interval of %ums", utf8Toutf16(__FUNCTION__), timerDef.sleepTime);
		} else {
			LOG_ERROR(L"%s() failed to start timer with interval %ums", utf8Toutf16(__FUNCTION__), timerDef.sleepTime);
		}
	}
#endif
}

#if USE_WINAPI_TIMERS
VOID CALLBACK wtwOTRmessaging::otrl_timer_callback(_In_  HWND hwnd, _In_  UINT uMsg, _In_  UINT_PTR idEvent, _In_  DWORD dwTime)
{
	otrl_message_poll(instance->getOtrlUserState(), &instance->itsOTRLops, nullptr);
}
#else
WTW_PTR	wtwOTRmessaging::otrl_timer_callback(WTW_PARAM wParam, WTW_PARAM lParam, void* opdata)
{
	//LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));

	wtwTimerEvent *event = reinterpret_cast<wtwTimerEvent*>(wParam);

	switch(event->event) {
		case WTW_TIMER_EVENT_CREATED:
			//LOG_INFO(L"%s() timer CREATED", utf8Toutf16(__FUNCTION__));
			break;
		case WTW_TIMER_EVENT_DESTROYED:
			LOG_INFO(L"%s() timer DESTROYED", utf8Toutf16(__FUNCTION__));
			break;
		case WTW_TIMER_EVENT_TICK:
			//LOG_INFO(L"%s() timer TICK", __FUNCTIONW__);
			otrl_message_poll(instance->getOtrlUserState(), &instance->itsOTRLops, 0);
			break;
		default:
			LOG_ERROR(L"%s() unhandle event", utf8Toutf16(__FUNCTION__));
	}

	return 0;
}
#endif


/*VOID wtwOTRmessaging::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);

	// TODO: uncomment when crash is gone

	if(OTRL_TIMER_ID == idEvent) {
	otrl_message_poll(instance->getOtrlUserState(), &itsOTRLops, 0);
	} else {
	MessageBox(NULL, L"FIXME: TimerProc - unknow timer id!", L"wtwOTRmessaging error", MB_OK);
	}

}*/



WTW_PTR wtwOTRmessaging::onProtocolEvent_cb(WTW_PARAM wParam, WTW_PARAM lParam, void*)
{
	const wtwProtocolEvent *proto = reinterpret_cast<wtwProtocolEvent*>(wParam);
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

	if ((proto->event == WTW_PEV_RAW_DATA_RECV) && (proto->type == WTW_PEV_TYPE_BEFORE))
	{
		const wtwRawDataDef* wtwData = reinterpret_cast<wtwRawDataDef*>(lParam);
		if (instance->itsSettingsBroker.getTraceIOMessages() &&
			(nullptr != wtwData) && (wtwData->pData))
		{
			try {
				LOG_TRACE(L"%s() RCVD RAW '%s'", __FUNCTIONW__,
					utf8Toutf16_safe(wtwData->pData, wtwData->pDataLen).operator LPCWSTR()
				);
			}
			catch (std::range_error &e) {
				LOG_TRACE(L"%s() RCVD RAW - EXCEPTION: %s", __FUNCTIONW__, utf8Toutf16(e.what()).operator LPCWSTR());
			}
		}
	}

	if ((proto->event == WTW_PEV_RAW_DATA_SEND) && (proto->type == WTW_PEV_TYPE_AFTER))
	{
		const wtwRawDataDef* wtwData = reinterpret_cast<wtwRawDataDef*>(lParam);
		if (instance->itsSettingsBroker.getTraceIOMessages() &&
			(nullptr != wtwData) && (wtwData->pData))
		{
			try {
				LOG_TRACE(L"%s() SENT RAW '%s'", __FUNCTIONW__,
						utf8Toutf16_safe(wtwData->pData, wtwData->pDataLen).operator LPCWSTR()
						);
			}
			catch (std::range_error &e) {
				LOG_TRACE(L"%s() SENT RAW - EXCEPTION: %s", __FUNCTIONW__, utf8Toutf16(e.what()).operator LPCWSTR());
			}
		}
	}

	if ((proto->event == WTW_PEV_MESSAGE_RECV) && (proto->type == WTW_PEV_TYPE_BEFORE))
	{
		const wtwMessageDef* wtwMsg = reinterpret_cast<wtwMessageDef*>(lParam);
		if (instance->itsSettingsBroker.getTraceIOMessages() &&
			(nullptr != wtwMsg) && (nullptr != wtwMsg->msgMessage))
		{
			LOG_TRACE(L"%s() RCVD flags 0x%08x '%s'", __FUNCTIONW__, wtwMsg->msgFlags, wtwMsg->msgMessage);
		}
	}

	if ((proto->event == WTW_PEV_MESSAGE_SEND) && (proto->type == WTW_PEV_TYPE_AFTER))
	{
		const wtwMessageDef* wtwMsg = reinterpret_cast<wtwMessageDef*>(lParam);
		if (instance->itsSettingsBroker.getTraceIOMessages() &&
			(nullptr != wtwMsg) && (nullptr != wtwMsg->msgMessage))
		{
			LOG_TRACE(L"%s() SENT '%s'", __FUNCTIONW__, wtwMsg->msgMessage);
		}
	}

	if ((proto->event == WTW_PEV_NETWORK_LOGOUT) /*&& (proto->type == WTW_PEV_TYPE_BEFORE)*/)
	{
		const wtwPresenceDef* wtwPres = reinterpret_cast<wtwPresenceDef*>(lParam);
		if (wtwPres && (wtwPres->flags & WTW_PRESENCE_FLAG_EXIT))
		{
			LOG_TRACE(L"%s() WTW_PRESENCE_FLAG_EXIT");

			// unfortunately, this PEV does not seem to work :(

			//endAllPrivateConversationsAtExit(netClass, netId);
		}
	}

	return S_OK;
}


WTW_PTR wtwOTRmessaging::onCELReceive_cb(WTW_PARAM wParam, WTW_PARAM lParam, void* userData)
{
	static int executed = 0;
	++executed;

	wtwCELMessage *celMsg = (wtwCELMessage*)wParam;

	addNetClassToSupported(celMsg->contactData.netClass);

	if (celMsg && celMsg->pText)
	{
		wtw::CString *msg = ((wtw::CString*)(celMsg->pText));
		//LOG_DEBUG(L"%s() - %s", __FUNCTIONW__, msg->operator LPCWSTR());

		char *newMsg = NULL;
		OtrlTLV *tlvs = NULL;

		/*
		MY_ConnContext_STRUCT * context = MY_ConnContext_STRUCT::getInstance();
		wcscpy_s(context->peer, _countof(context->peer), celMsg->contactData.id);
		wcscpy_s(context->netClass, _countof(context->netClass), celMsg->contactData.netClass);
		context->netId = celMsg->contactData.netId;
		*/

		WtwOtrContext *wtwOtrContext = WtwOtrContext::find(celMsg->contactData);
		if (nullptr == wtwOtrContext) {
			LOG_TRACE(L"%s() WtwOtrContext find failed", __FUNCTIONW__);
		}

		char accountname[20];
		WtwOtrContext::accountnameFromNetId(accountname, sizeof(accountname), celMsg->contactData.netId);
		const wchar_t *origMsg = msg->operator LPCWSTR();

		int ignoreMsg = otrl_message_receiving(instance->itsOtrlUserState, &itsOTRLops, wtwOtrContext,
			accountname, utf16Toutf8(celMsg->contactData.netClass),
			utf16Toutf8(celMsg->contactData.id), utf16Toutf8(origMsg), &newMsg,
			&tlvs, NULL, otrl_ConnContex_created, NULL);

		// Check if other side has ended the secure connection
		{
			wchar_t msg[2 * 1024];
			OtrlTLV *tlv = otrl_tlv_find(tlvs, OTRL_TLV_DISCONNECTED);
			if (tlv) {
				/* Notify the user that the other side disconnected. */
				StringCbPrintfW(msg, sizeof(msg), L"Prywatna rozmowa z u�ytkownikiem '%s' zosta�a zako�czona", celMsg->contactData.id);
				instance->displayMsgInChat(celMsg->contactData.id, celMsg->contactData.netClass,
					celMsg->contactData.netId, msg, WTW_MESSAGE_FLAG_INFO);

				// open window so that button gets refreshed
				wtwPf->fnCall(WTW_FUNCT_OPEN_CHAT_WINDOW, reinterpret_cast<WTW_PARAM>(&celMsg->contactData), 0);

				ChatBroker::update_ui(nullptr);
			}
			otrl_tlv_free(tlvs);
		}

		LOG_TRACE(L"%s() executed: %d, ignore: %u, newMsg: %u, msg: '%s'", __FUNCTIONW__,
			executed, (unsigned)(0 != ignoreMsg), (unsigned)(NULL != newMsg), origMsg);

		if (ignoreMsg)
		{
			return WTW_CEL_RET_CONSUME;
		}
		else
		{
			if (newMsg)
			{
				//pWtwMessage->msgFlags |= WTW_MESSAGE_FLAG_ENCRYPTED | WTW_MESSAGE_FLAG_NOARCHIVE;

				utf16String u16_newMsg = utf8Toutf16(newMsg);

				if (instance->itsSettingsBroker.getRemoveHtmlTags())
				{
					u16_newMsg = u16_newMsg.htmlTagsStripped();
				}
				if (instance->itsSettingsBroker.getReplaceHtmlEntities())
				{
					u16_newMsg = u16_newMsg.htmlEntitiesStripped();
				}

				*msg = u16_newMsg;

				LOG_TRACE(L"%s() decrypted msg '%s'", __FUNCTIONW__, u16_newMsg.operator LPCWSTR());
				LOG_TRACE(L"%s() celMsg->flags 0x%08x", __FUNCTIONW__, celMsg->flags);

				if (0 == (WTW_MESSAGE_FLAG_NOTHEME & celMsg->flags)) {
					//celMsg->flags &= ~WTW_MESSAGE_FLAG_NOTHEME;
					//LOG_TRACE(L"%s() celMsg->flags 0x%08x  WTW_MESSAGE_FLAG_NOTHEME cleared", __FUNCTIONW__, celMsg->flags);

					celMsg->flags |= WTW_MESSAGE_FLAG_NOTHEME;
					LOG_TRACE(L"%s() celMsg->flags 0x%08x  WTW_MESSAGE_FLAG_NOTHEME set", __FUNCTIONW__, celMsg->flags);
				}
				

				return WTW_CEL_RET_DECRYPTED;
			}
			else {
				//LOG_DEBUG(L"pusty pointer newMsg! nieszyfrowana wiadomosc ktora trzeba przekazac dalej");
			}
		}
	}

	return WTW_CEL_RET_OK;
}

WTW_PTR wtwOTRmessaging::onCELBeforeSend_cb(WTW_PARAM wParam, WTW_PARAM lParam, void* userData)
{
	wtwCELMessage *celMsg = (wtwCELMessage*)wParam;

	addNetClassToSupported(celMsg->contactData.netClass);

	if (celMsg && celMsg->pText)
	{
		wtw::CString *msg = (wtw::CString*)(celMsg->pText);
		//LOG_DEBUG(L"%s() 0x%x - %s", __FUNCTIONW__, celMsg->flags, msg->operator LPCWSTR());

		/*
		MY_ConnContext_STRUCT * context = MY_ConnContext_STRUCT::getInstance();
		wcscpy_s(context->peer, _countof(context->peer), celMsg->contactData.id);
		wcscpy_s(context->netClass, _countof(context->netClass), celMsg->contactData.netClass);
		context->netId = celMsg->contactData.netId;
		*/

		WtwOtrContext *wtwOtrContext = WtwOtrContext::find(celMsg->contactData);
		if (nullptr == wtwOtrContext) {
			LOG_TRACE(L"%s() WtwOtrContext find failed", __FUNCTIONW__);
		}

		char accountname[20];
		WtwOtrContext::accountnameFromNetId(accountname, sizeof(accountname), celMsg->contactData.netId);

		const std::string unencryptedmsg (utf16Toutf8(msg->operator LPCWSTR()));
		char *newmessage = NULL;
		gcry_error_t err = otrl_message_sending(instance->getOtrlUserState(),
			&instance->itsOTRLops,
			wtwOtrContext, //opdata
			accountname, //accountname
			utf16Toutf8(celMsg->contactData.netClass), //protocol
			utf16Toutf8(celMsg->contactData.id), //username
			OTRL_INSTAG_BEST, //instance,
			unencryptedmsg.c_str(),
			NULL, //OtrlTLV*
			&newmessage,
			OTRL_FRAGMENT_SEND_ALL_BUT_LAST,
			NULL, otrl_ConnContex_created, NULL);

		LOG_TRACE(L"%s() err: %u, newMsg: %u, msg: '%s', newMsg: '%s'", __FUNCTIONW__,
			err, (unsigned)(nullptr != newmessage), msg->operator LPCWSTR(),
			(newmessage ? utf8Toutf16(newmessage).operator LPCWSTR() : L"(nullptr)"));

		if (GPG_ERR_NO_ERROR == err) {
			if (0 < strnlen_s(newmessage, 2)) // strnlen_s() return 0 if newmessage==NULL
			{
				*msg = utf8Toutf16(newmessage);

				LOG_TRACE(L"WTW_CEL_RET_ENCRYPTED newmsg: '%s'",
					//#ifdef _WTWOTRMESSAGING_USE_DYNAMIC_STRINGS
						utf8Toutf16_safe(newmessage, strnlen_s(newmessage, 4321)).operator LPCWSTR()
					//#else
					//	utf8Toutf16_safe(newmessage, strnlen_s(newmessage, 4321))
					//#endif
						);

				otrl_message_free(newmessage);
				
				// below does not prevent from archving
				//celMsg->flags |= WTW_MESSAGE_FLAG_NOARCHIVE;
				return WTW_CEL_RET_ENCRYPTED;
			}
			else {
				// empty message - it may happen when we are trying to send a message
				// which should not be encrypted at all

				// context had not existed before otrl_message_sending was called,
				// but it could be created
				wtwOtrContext = (nullptr != wtwOtrContext) ? wtwOtrContext : WtwOtrContext::find(celMsg->contactData);

				if ((nullptr != wtwOtrContext) && (wtwOtrContext->otr.privateConverstationEnded))
				{
					LOG_TRACE(L"%s() msg cleared due to privateConversationEnded", __FUNCTIONW__);
					*msg = L""; // clear the message
					return WTW_CEL_RET_ENCRYPTED; // only this status make above replacement effective
				}
			}
		}
		else {
			LOG_ERROR(L"Error occured while trying to cipher the message");
			instance->displayMsgInChat(celMsg->contactData.id,
				celMsg->contactData.netClass, celMsg->contactData.netId,
				L"Failed to cipher your message - it has NOT been sent!", WTW_MESSAGE_FLAG_ERROR);
		}
	}

	return WTW_CEL_RET_OK;
}



void wtwOTRmessaging::setInstancePointer()
{
	if (nullptr == instance) {
		instance = this;
	}
	else {
		LOG_CRITICAL(L"Second instance of wtwOTRmessaging cannot be created!");
		//MessageBox( NULL, L"Second instance of wtwOTRmessaging cannot be created!",
		//			L"wtwOTRmessaging", MB_ICONEXCLAMATION );
		return;
	}
}


void wtwOTRmessaging::unsetInstancePointer() {
	if (nullptr != instance) {
		instance = 0;
	}
	else {
		LOG_CRITICAL(L"wtwOTRmessaging instance pointer is null!");
		//MessageBox( NULL, L"wtwOTRmessaging instance does not exist!",
		//			L"wtwOTRmessaging error", MB_ICONEXCLAMATION );
	}
}


void wtwOTRmessaging::initializeOTRL()
{
	OTRL_INIT;

	itsOtrlUserState = otrl_userstate_create();

	// Read stored private keys if file exists
	DWORD dwAttrib = GetFileAttributes(itsSettingsBroker.getPrivateKeyFullPath());
	if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
		if (GPG_ERR_NO_ERROR != otrl_privkey_read(itsOtrlUserState,
							utf16Toutf8(itsSettingsBroker.getPrivateKeyFullPath()))) {
			LOG_CRITICAL(L"Failed to read private key from file!");
		}
	}

	// Read stored instance tags:
	if (GPG_ERR_NO_ERROR == otrl_instag_read(itsOtrlUserState,
							utf16Toutf8(itsSettingsBroker.getInstagFileFullPath()))) {
		//LOG_DEBUG(L"initializeOTRL() instance tags read successfully");
	}
	else {
		LOG_WARN(L"initializeOTRL() instance tags read failed");
	}

	// TODO: uncomment
	// Read stored fingerprints
	/*  add_app_info is a function that will be called in the event that a new
	 *  ConnContext is created.It will be passed the add_app_info_data that
	 *	you supplied, as well as a pointer to the new ConnContext.You can use
	 *	this to add application - specific information to the ConnContext using
	 *	the "context->app" field, for example.If you don't need to do this,
	 *	you can pass NULL for the last two arguments of
	 *	otrl_privkey_read_fingerprints.
	 */
	if (GPG_ERR_NO_ERROR == otrl_privkey_read_fingerprints(itsOtrlUserState,
						utf16Toutf8(itsSettingsBroker.getFigerprintFileFullPath()),
						otrl_ConnContex_created, nullptr))
	{
		//LOG_DEBUG(L"initializeOTRL() read fingerprints successfully");
	} else {
		LOG_TRACE(L"%s() read fingerprints failed (%s)", __FUNCTIONW__,
			itsSettingsBroker.getFigerprintFileFullPath());
	}
}


void wtwOTRmessaging::releaseOTRL()
{
	// destroy timer if running/set
	// NOTE: timer alrady stopped in ~wtwOTRmessaging()

	otrl_userstate_free(itsOtrlUserState);
	itsOtrlUserState = nullptr;
}


void wtwOTRmessaging::installWtwHooks()
{
//	onBeforeMsgDisp2_hook = wtwPf->evHook(WTW_EVENT_CHATWND_BEFORE_MSG_DISP2,
//		wtwOTRmessaging::onBeforeMsgDisp2_cb, this);

	onProtocolEvent_hook = wtwPf->evHook(WTW_ON_PROTOCOL_EVENT,
		wtwOTRmessaging::onProtocolEvent_cb, this);

//	onChatwndBeforeMsgProc_hook = wtwPf->evHook(WTW_EVENT_CHATWND_BEFORE_MSG_PROC,
//		wtwOTRmessaging::onChatwndBeforeMsgProc_cb, this);

	onCELReceive_hook = wtwPf->evHook(WTW_CEL_MESSAGE_RECEIVED,
		wtwOTRmessaging::onCELReceive_cb, this);

	onCELBeforeSend_hook = wtwPf->evHook(WTW_CEL_MESSAGE_BEFORE_SEND,
		wtwOTRmessaging::onCELBeforeSend_cb, this);
}


void wtwOTRmessaging::removeWtwHooks()
{
	wtwPf->evUnhook(onCELBeforeSend_hook);
	wtwPf->evUnhook(onCELReceive_hook);
//	wtwPf->evUnhook(onChatwndBeforeMsgProc_hook);
	wtwPf->evUnhook(onProtocolEvent_hook);
//	wtwPf->evUnhook(onBeforeMsgDisp2_hook);
}


void wtwOTRmessaging::installWtwFunctions()
{
	//LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);

	//handler = wtwInstProtoFunc(wtw, PROTO_NAME, netSID, WTW_PF_MESSAGE_SEND, &funcMessageSend, 0);
}


void wtwOTRmessaging::removeWtwFunctions()
{
	//LOG_ERROR(L"%s() not implemented", __FUNCTIONW__);
	// TODO: implement it :)
}

void wtwOTRmessaging::endAllPrivateConversationsAtExit()
{
	/*
	const WtwOtrContext *contexts = WtwOtrContext::getAllInstances();
	for (int i = 0; i < WtwOtrContext::INSTANCES_TOTAL; ++i)
	{
		const WtwOtrContext &c = contexts[i];
		if (c.inUse)
		{
			todo
		}
	}
	*/

	for (auto context = wtwOTRmessaging::instance->getOtrlUserState()->context_root;
		context != nullptr; context = context->next)
	{
		// skip child context
		if (context->m_context != context)
			continue;

		if (nullptr != context)
		{
			if ((nullptr != context->recent_child) && (OTRL_MSGSTATE_ENCRYPTED == context->recent_child->msgstate))
			{
				WtwOtrContext *c = WtwOtrContext::find(context->username, context->protocol, context->accountname);
				if (nullptr != c)
				{
					LOG_TRACE(L"%s() finishing private conversation with %s/%s/%d", __FUNCTIONW__,
						c->wtw.id, c->wtw.netClass, c->wtw.netId);
					
					otrl_message_disconnect(instance->getOtrlUserState(),
						&instance->itsOTRLops,
						c,
						context->accountname,
						context->protocol,
						context->username,
						OTRL_INSTAG_RECENT);
				}
			}
		}
	}
}

int wtwOTRmessaging::sendRawMessageToNetwork(const char* msg)
{
	wtwProtocolEvent ev;
	initStruct(ev);

	ev.event = WTW_PEV_RAW_DATA_SEND;
	ev.netClass = L"GG"; //netClass;
	ev.netId = 1; //netId; 

	wtwRawDataDef rawData;
	initStruct(rawData);
	rawData.pData = msg;
	rawData.pDataLen = strlen(msg);
	rawData.flags = WTW_RAW_FLAG_EXT; // EXTernal data

	ev.type = WTW_PEV_TYPE_BEFORE;
	if (wtwPf->fnCall(WTW_PF_CALL_HOOKS, reinterpret_cast<WTW_PARAM>(&ev), reinterpret_cast<WTW_PARAM>(&rawData)) == 0)
	{
		ev.type = WTW_PEV_TYPE_AFTER;
		wtwPf->fnCall(WTW_PF_CALL_HOOKS, reinterpret_cast<WTW_PARAM>(&ev), reinterpret_cast<WTW_PARAM>(&rawData));
		return 0;
	}

	return 1;
}


int wtwOTRmessaging::sendRawMessageToContact(const wchar_t *id, const wchar_t *netClass, int netId,
	const wchar_t *msg, unsigned int extraMsgFlags)
{
	//LOG_INFO(L"%s() wyslam raw msg: %s", __FUNCTIONW__, msg);

	ASSERT(nullptr != id);
	ASSERT(nullptr != netClass);

	wtwMessageDef msgStruct;
	//initStruct(msgStruct);
	msgStruct.contactData.id = id;
	msgStruct.contactData.netClass = netClass;
	msgStruct.contactData.netId = netId;
	msgStruct.msgFlags = extraMsgFlags;
	msgStruct.msgMessage = msg;

	wchar_t _f[255] = { 0 };
	StringCbPrintfW(_f, sizeof(_f), L"%s/%d/%s", netClass, netId, WTW_PF_MESSAGE_SEND);
	wtwPf->fnCall(_f, (WTW_PARAM)(&msgStruct), 0);

	return 0;
}


int wtwOTRmessaging::otrg_plugin_send_default_query_conv(wtwContactDef *contact)
{
	char *msg;
	int ret = 0;

	if (false == isNetClassSupported(contact->netClass))
	{
		wchar_t buf[255];
		StringCbPrintf(buf, sizeof(buf) / sizeof(wchar_t),
			L"Protok�/wtyczka %s nie wspiera odpowiednich mechanizm�w szyfrowania.", contact->netClass);
		instance->displayMsgInChat(contact->id, contact->netClass, contact->netId, buf, WTW_MESSAGE_FLAG_INFO);
		return -1;
	}

	// TODO: replace with current user login or name
	msg = otrl_proto_default_query_msg("name", OTRL_POLICY_DEFAULT);

	if (msg) {
		ret = instance->sendRawMessageToContact(contact->id, contact->netClass, contact->netId, utf8Toutf16(msg),
												WTW_MESSAGE_FLAG_OUTGOING | WTW_MESSAGE_FLAG_CHAT_MSG);
		free(msg);
	}
	else
	{
		LOG_ERROR(L"%s() otrl_proto_default_query_msg() returned nullptr!", __FUNCTIONW__);
	}

	return ret;
}


int wtwOTRmessaging::otrg_finish_private_conversation(wtwContactDef *contact)
{
	const wchar_t *flagClearStr = L"";
	WtwOtrContext *wtwOtrContext = WtwOtrContext::find(*contact);
	if ((nullptr != wtwOtrContext) && (true == wtwOtrContext->otr.privateConverstationEnded))
	{
		flagClearStr = L"clearing 'privateConverstationEnded' flag";
		wtwOtrContext->otr.privateConverstationEnded = false;
	}

	LOG_TRACE(L"%s() %s/%s/%d %s", __FUNCTIONW__, contact->id, contact->netClass, contact->netId, flagClearStr);

	char accountname[20];
	WtwOtrContext::accountnameFromNetId(accountname, sizeof(accountname), contact->netId);

	otrl_message_disconnect(instance->getOtrlUserState(),
		&instance->itsOTRLops,
		wtwOtrContext,
		accountname,
		utf16Toutf8(contact->netClass),
		utf16Toutf8(contact->id),
		OTRL_INSTAG_BEST);

	wchar_t s[500];
	StringCbPrintfW(s, sizeof(s), L"Zako�czono prywatn� rozmow� z u�ytkownikiem '%s'", contact->id);
	instance->displayMsgInChat(contact->id, contact->netClass, contact->netId, s, WTW_MESSAGE_FLAG_INFO);

		// open window so that button gets refreshed
		wtwPf->fnCall(WTW_FUNCT_OPEN_CHAT_WINDOW, reinterpret_cast<WTW_PARAM>(contact), 0);
	ChatBroker::update_ui(nullptr);

	return 0;
}


void wtwOTRmessaging::displayMsgInChat(const wchar_t *id, const wchar_t *netClass, int netId,
	const wchar_t *msg, DWORD wtwMsgFlags, bool fontBold, bool tooltip, bool archiveMsg)
{
	const int max_len = 2048;
	size_t len;
	bool len_ok = false;

	if (S_OK == StringCchLength(msg, max_len+1, &len)) {
		if (len < max_len) {
			len_ok = true;
		} else {
			LOG_ERROR(L"%s() string is too long %u (max 2000)", __FUNCTIONW__, (unsigned)len);
		}
	}

	if (nullptr != id && nullptr != msg && true == len_ok)
	{
		wchar_t decorated[2048 + 512];
		StringCbPrintfW(decorated, sizeof(decorated),
			L"<span %s %s>[wtwOTRmessaging] %s</span>",
			(fontBold) ? L"style=\"font-weight:bold;\"" : L"",
			(tooltip) ? L"title=\"Wiadomo�� od wtyczki wtwOTRmessaging\"" : L"",
			msg);

		wtwMessageDef md;
		initStruct(md);
		md.contactData.id = id;
		md.contactData.netClass = netClass;
		md.contactData.netId = netId;
		md.msgMessage = decorated;
		md.msgTime = time(NULL);
		md.msgFlags = WTW_MESSAGE_FLAG_CHAT_MSG | WTW_MESSAGE_FLAG_NOHTMLESC |
			(wtwMsgFlags & (WTW_MESSAGE_FLAG_INFO | WTW_MESSAGE_FLAG_WARNING | WTW_MESSAGE_FLAG_ERROR) ) |
			((false == archiveMsg) ? WTW_MESSAGE_FLAG_NOARCHIVE : 0); // do not archive plugin messages
		wtwPf->fnCall(WTW_CHATWND_SHOW_MESSAGE, TO_WTW_PARAM(&md), 0);
	}
}

void wtwOTRmessaging::displayMsgInChat(const WtwOtrContext* wtwOtrContext,
	const wchar_t *msg, DWORD wtwMsgFlags, bool fontBold, bool tooltip, bool archiveMsg)
{
	if (nullptr != wtwOtrContext)
	{
		ASSERT(nullptr != wtwOtrContext->wtw.id);
		ASSERT(nullptr != wtwOtrContext->wtw.netClass);
		displayMsgInChat(wtwOtrContext->wtw.id, wtwOtrContext->wtw.netClass,
			wtwOtrContext->wtw.netId, msg, wtwMsgFlags, fontBold, tooltip, archiveMsg);
	}
	else
	{
		LOG_CRITICAL(L"%s() wtwContact is nullptr (msg: %s)", __FUNCTIONW__, msg);
	}
}


bool wtwOTRmessaging::isNetClassSupported(const wchar_t* netClass)
{
	for (std::vector<CString>::iterator it = supportedNetClasses.begin(); it != supportedNetClasses.end(); ++it)
	{
		if (!it->IsEmpty() && (0 == wcsncmp(netClass, it->operator LPCWSTR(), it->GetLength()))) {
			return true;
		}
	}
	return false;
}


void wtwOTRmessaging::addNetClassToSupported(const wchar_t *netClass)
{
	if (std::find(supportedNetClasses.begin(), supportedNetClasses.end(), netClass) == supportedNetClasses.end())
	{
		LOG_INFO(L"%s() adding netClass '%s' to supported list", __FUNCTIONW__, netClass);
		supportedNetClasses.emplace_back(netClass);
	}
}


WtwOtrContext* WtwOtrContext::find(const wtwContactDef &contact)
{
	for (int i = 0; i < INSTANCES_TOTAL; i++)
	{
		if ((true == _instances[i].inUse) &&
			(contact.netId == _instances[i].wtw.netId) &&
			(0 == wcsncmp(contact.id, _instances[i].wtw.id, _countof(_instances[i].wtw.id))) &&
			(0 == wcsncmp(contact.netClass, _instances[i].wtw.netClass, _countof(_instances[i].wtw.netClass))))
		{
			return &_instances[i];
		}
	}
	return nullptr;
}

WtwOtrContext* WtwOtrContext::find(const char *username, const char *protocol, const char *accountname)
{
	for (int i = 0; i < INSTANCES_TOTAL; i++)
	{
		if ((true == _instances[i].inUse) &&
			(0 == strncmp(username, _instances[i].otr.username, STR_LEN)) &&
			(0 == strncmp(protocol, _instances[i].otr.protocol, STR_LEN)) &&
			(0 == strncmp(accountname, _instances[i].otr.accountname, STR_LEN)) )
		{
			return &_instances[i];
		}
	}
	return nullptr;
}

void WtwOtrContext::accountnameFromNetId(char *buf, size_t sizeOfBuf, int netId)
{
	_snprintf_s(buf, sizeOfBuf, sizeOfBuf, "%d", netId);

	/*******************************************************************************
	**
	** Function:
	**		WTW_PROTO_FUNC_GET
	**
	** Return value:
	**		on error:
	**				E_*
	**		on success:
	**				S_OK
	**
	** Parameters:
	**		wP - (const wchar_t*) - GUID
	**		lP - (wtwProtocolInfo*)
	**
	*/

	//#define WTW_PROTO_FUNC_GET		L"WTW/ProtocolGet"		// wP - wchar_t* - GUID, lP - wtwProtocolInfo*; zwraca E_*/S_OK

	/*******************************************************************************
	**
	** Function:
	**		WTW_PROTO_FUNC_GET_GUID_EX
	**
	** Return value:
	**		on error:
	**				E_*
	**		on success:
	**				S_OK
	**
	** Parameters:
	**		wP - (wtwProtocolInfo*) - with netClass and netId set
	**		lP - (wtwBuffer*)		- with info about allocated pointer with enough space to hold the GUID
	**
	*/

	//#define WTW_PROTO_FUNC_GET_GUID_EX	L"WTW/ProtocolGetGUIDEx"
}

int WtwOtrContext::netIdFromAccountname(const char *buf)
{
	return atoi(buf);
}

const WtwOtrContext *WtwOtrContext::getAllInstances()
{
	return _instances;
}
