// wtwOTRmessaging.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "wtwOTRmessaging.h"
#include "Logger.h"
#include "utils.h"
#include "Resource.h" // version information
#include <Strsafe.h> // StringCchPrintW()

#include <clocale> // std::set_locale()
#include <cstdlib> // std::wctombs()
#include <codecvt> // std::codecvt

#include <locale>

extern "C" {
	//#include "libotr/src/message.h"
	#include "OTRL/privkey.h"
}

//#pragma comment(lib, "user32.lib")

int MY_wtwMessageDef_STRUCT::_instance_index = 0;
MY_wtwMessageDef_STRUCT MY_wtwMessageDef_STRUCT::_instances[16];

static wtwOTRmessaging * p_wtwOTRmessaging = nullptr;

wtwOTRmessaging *wtwOTRmessaging::instance = nullptr;
//UINT_PTR wtwOTRmessaging::OTRL_timer_id = 0;

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
	sizeof(WTWPLUGINFO),									// rozmiar struktury
	L"wtwOTRmessaging",										// nazwa wtyczki
	L"Implementacja protoko³u Off-the-Record Messaging, "
	L"który umo¿liwia szyfrowanie oraz uwierzytelnianie rozmów.", // opis wtyczki
	L"(c) 2014",											// copyrights
	L"blad3master",											// autor
	L"blad3master@blad3master.com",							// email
	L"http://blad3master.com",								// strona www
	L"http://blad3master.com/download/wtwOTRmessaging.xml",	// link do pliku AU, podac 0 jezeli nie uzywamy AU
	PLUGIN_API_VERSION,										// wersja api z ktora zostal skompilowany plug
	MAKE_QWORD(RELEASE_VER_MAJOR, RELEASE_VER_MINOR,
				RELEASE_VER_BUILD, RELEASE_VER_REVISION),	// wersja plug'a
	WTW_CLASS_UTILITY,										// klasa plugin'a
	0,														// fcja ktora wtw wywola jezeli user bedzie chcial zobaczyc about plug'a
	L"{2195C31D-89D8-41bb-A390-622E88DB476B}",				// GUID, wymagane...
	0,													    // zaleznosci (tablica GUID'ow, ostatnia pozycja MA byc 0)
	0, 0, 0, 0												// zarezerwowane
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

	wtw = f;
	p_wtwOTRmessaging = new wtwOTRmessaging();

	//MessageBox(NULL, L"Wtyczka za³adowana raz", L"Hello world!", MB_OK);
	return 0;
}


WTW_PLUGIN_API_ENTRY(int) pluginUnload(DWORD /*callReason*/)
{
	if (nullptr != p_wtwOTRmessaging)
		delete p_wtwOTRmessaging;

	wtw = 0;
	hInstance = NULL;

	//MessageBox(NULL, L"Wtyczka wy³adowana", L"Hello world!", MB_OK);
	return 0;
}


#pragma warning(disable: 4355) /* 'this' : used in base member initializer list */
wtwOTRmessaging::wtwOTRmessaging() :
	itsSettingsBroker(this)
#pragma warning(default: 4355)
{
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
}


OtrlPolicy wtwOTRmessaging::OTRL_policy_cb(void *opdata, ConnContext *context)
{
	//LOG_DEBUG(L"%s()\naccountname:'%s'\nprotocol : '%s'\nusername : '%s'\n", __FUNCTIONW__,
	//	utf8Toutf16(context->accountname), utf8Toutf16(context->protocol), utf8Toutf16(context->username));

	//TODO: return policy set in gui(settings) and return it instead
	// of default value

	OtrlPolicy policy = OTRL_POLICY_DEFAULT;
	return policy;
}


void wtwOTRmessaging::OTRL_create_privkey_cb(void *opdata, const char *accountname, const char *protocol)
{
	LOG_INFO(L"%s() accountname:'%s', protocol:'%s'", utf8Toutf16(__FUNCTION__),
		utf8Toutf16(accountname), utf8Toutf16(protocol));

	OtrlPrivKey * key = 0;

	if (0 != otrl_privkey_find(instance->getOtrlUserState(), accountname, protocol))
	{
		LOG_WARN(L"%s() Private key already exists", utf8Toutf16(__FUNCTION__));
		MessageBox(nullptr, L"Private key for the user already exists", L"wtwOTRmessaging", MB_ICONINFORMATION);
		return;
	}

	const char *pk_file_path = utf16Toutf8(instance->itsSettingsBroker.getPrivateKeyFullPath());
	if (0 == pk_file_path[0])
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
		if (GPG_ERR_NO_ERROR == otrl_privkey_generate_finish(instance->getOtrlUserState(), key, pk_file_path))
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
	LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
	return 1;
}


void wtwOTRmessaging::OTRL_inject_message_cb(void *opdata, const char *accountname, const char *protocol,
	const char *recipient, const char *message)
{
	//LOG_DEBUG(L"SENDING RAW MSG to contact:");
	//LOG_DEBUG(L"%s", utf8Toutf16(message));
	//LOG_DEBUG(L"----------------------------------------------------");

	
#if 1
	// Sending to contact triggers many hooks
	wtwContactDef contact;
	initStruct(contact);
	contact.id = utf8Toutf16(recipient);
	contact.netClass = utf8Toutf16(protocol);
	contact.netId = 1;
	sendRawMessageToContact(&contact, utf8Toutf16(message),
		WTW_MESSAGE_FLAG_OUTGOING | WTW_MESSAGE_FLAG_CHAT_MSG /*| WTW_MESSAGE_FLAG_ENCRYPTED*/);
#else
	// Sending to networks mayb would allow to skip some unnecessary hooks
	//sendRawMessageToNetwork(message);
#endif
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
	ChatBroker::update_ui();
}


void wtwOTRmessaging::OTRL_new_fingerprint_cb(void *opdata, OtrlUserState us,
	const char *accountname, const char *protocol, const char *username, unsigned char fingerprint[20])
{
	wchar_t s[500];
	char fp_string[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];

	otrl_privkey_hash_to_human(fp_string, fingerprint);
	
	StringCbPrintfW(s, sizeof(s), L"(AccountName: %s Protocol: %s) Otrzymano nowy klucz od u¿ytkownika '%s', odcisk '%s'",
		utf8Toutf16(accountname), utf8Toutf16(protocol),
		utf8Toutf16(username), utf8Toutf16(fp_string));

	instance->displayMsgInChat(utf8Toutf16(username), s);

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
	//LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));

	wchar_t s[500];
	StringCbPrintfW(s, sizeof(s), L"Rozpoczêto prywatn¹ rozmowê z u¿ytkownikiem '%s'",
		utf8Toutf16(context->username));

	instance->displayMsgInChat(utf8Toutf16(context->username), s);

	ChatBroker::update_ui();

	

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
	StringCbPrintfW(s, sizeof(s), L"Pomyœlnie odœwie¿ono prywatn¹ rozmowê z u¿ytkownikiem '%s'",
		utf8Toutf16(context->username));
	wtwOTRmessaging::instance->displayMsgInChat(utf8Toutf16(context->username), s);
}


int wtwOTRmessaging::OTRL_max_message_size_cb(void *opdata, ConnContext *context) {
	//LOG_WARN(L"%s() not implemented - returns hardcoded value", utf8Toutf16(__FUNCTION__));
	
	// TODO: create an array of known protocols->MaxMessageSize mappings
	return 500;
}



void wtwOTRmessaging::OTRL_received_symkey_cb(void *opdata,	ConnContext *context, unsigned int use,
						const unsigned char *usedata, size_t usedatalen, const unsigned char *symkey)
{
	LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
}


const char *wtwOTRmessaging::OTRL_otr_error_message_cb(void *opdata, ConnContext *context, OtrlErrorCode err_code)
{
	LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
	return 0;
}


void wtwOTRmessaging::OTRL_otr_error_message_free_cb(void *opdata, const char *err_msg)
{
	LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
}


const char *wtwOTRmessaging::OTRL_resent_msg_prefix_cb(void *opdata, ConnContext *context)
{
	LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
	return 0;
}


void wtwOTRmessaging::OTRL_resent_msg_prefix_free_cb(void *opdata, const char *prefix)
{
	LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
}


void wtwOTRmessaging::OTRL_handle_smp_event_cb(void *opdata, OtrlSMPEvent smp_event, ConnContext *context,
												unsigned short progress_percent, char *question)
{
	LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
}


void wtwOTRmessaging::OTRL_handle_msg_event_cb(void *opdata, OtrlMessageEvent msg_event, ConnContext *context,
												const char *message, gcry_error_t err)
{
	//LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));

	wchar_t msg[2048];
	const wchar_t *peer_name = utf8Toutf16(context->username);

	switch (msg_event)
	{
	case OTRL_MSGEVENT_ENCRYPTION_REQUIRED:
		StringCbPrintfW(msg, sizeof(msg),
			L"Próbowa³eœ wys³aæ niezaszyfrowan¹ wiadomoœæ do '%s'",
			peer_name);		
		instance->displayMsgInChat(peer_name, msg);

		/*display_otr_message_or_notify(opdata, context->accountname,
			context->protocol, context->username, _("Attempting to"
			" start a private conversation..."), 1, OTRL_NOTIFY_WARNING,
			_("OTR Policy Violation"), buf,
			_("Unencrypted messages to this recipient are "
			"not allowed.  Attempting to start a private "
			"conversation.\n\nYour message will be "
			"retransmitted when the private conversation "
			"starts."));
		*/
		break;
//	case OTRL_MSGEVENT_ENCRYPTION_ERROR:
//		break;
	case OTRL_MSGEVENT_CONNECTION_ENDED:
		StringCbPrintfW(msg, sizeof(msg),
			L"Wiadomoœæ do u¿ytkownika '%s' NIE zosta³a wys³ana. "
			L"Zakoñcz prywatn¹ rozmowê lub ponowniê j¹ rozpocznij.",
			peer_name);
		instance->displayMsgInChat(peer_name, msg);
		
		ChatBroker::update_ui();
		break;
//	case OTRL_MSGEVENT_SETUP_ERROR:
//		break;
//	case OTRL_MSGEVENT_MSG_REFLECTED:
//		break;
//	case OTRL_MSGEVENT_MSG_RESENT:
//		break;
//	case OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE:
//		break;
//	case OTRL_MSGEVENT_RCVDMSG_UNREADABLE:
//		break;
//	case OTRL_MSGEVENT_RCVDMSG_MALFORMED:
//		break;
	case OTRL_MSGEVENT_LOG_HEARTBEAT_RCVD:
		// Do not print information about hearbit since it is "detected"
		// when the peer finishes private conversation - the message might
		// be confusing for the user :)
		/*
		StringCbPrintfW(msg, sizeof(msg),
			L"Otrzymano heartbeat od u¿ytkownika '%s'",
			peer_name);
		instance->displayMsgInChat(peer_name, msg);
		*/
		break;
	case OTRL_MSGEVENT_LOG_HEARTBEAT_SENT:
		break;
//	case OTRL_MSGEVENT_RCVDMSG_GENERAL_ERR:
//		break;
	case OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED:
		StringCbPrintfW(msg, sizeof(msg),
			L"<b>The following message received from '%s' was <i>not</i> encrypted: [</b>%s<b>]</b>",
			peer_name, utf8Toutf16(message));
		instance->displayMsgInChat(peer_name, msg, false);
		break;
//	case OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED:
//		break;
//	case OTRL_MSGEVENT_RCVDMSG_FOR_OTHER_INSTANCE:
//		break;

/*
	case 100:
		StringCbPrintfW(msg, sizeof(msg),
			L"Prywatna rozmowa z u¿ytkownikiem '%s' zosta³a zakoñczona",
			peer_name);
		instance->displayMsgInChat(peer_name, msg);

		ChatBroker::update_ui();
		break;
*/

	default:
		LOG_WARN(L"%s() - handling of event %d is not yet implemented",
			utf8Toutf16(__FUNCTION__), (int)msg_event);
	}
	

/*	PIDGIN plugin implementation

	PurpleConversation *conv = NULL;
    gchar *buf;
    OtrlMessageEvent * last_msg_event;

    if (!context) return;

    conv = otrg_plugin_context_to_conv(context, 1);
    last_msg_event = g_hash_table_lookup(conv->data, "otr-last_msg_event");

    switch (msg_event)
    {
	case OTRL_MSGEVENT_NONE:
	    break;
	case OTRL_MSGEVENT_ENCRYPTION_REQUIRED:
	    buf = g_strdup_printf(_("You attempted to send an "
		    "unencrypted message to %s"), context->username);
	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, _("Attempting to"
		    " start a private conversation..."), 1, OTRL_NOTIFY_WARNING,
		    _("OTR Policy Violation"), buf,
		    _("Unencrypted messages to this recipient are "
		    "not allowed.  Attempting to start a private "
		    "conversation.\n\nYour message will be "
		    "retransmitted when the private conversation "
		    "starts."));
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_ENCRYPTION_ERROR:
	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, _("An error occurred "
		    "when encrypting your message.  The message was not sent."),
		    1, OTRL_NOTIFY_ERROR, _("Error encrypting message"),
		    _("An error occurred when encrypting your message"),
		    _("The message was not sent."));
	    break;
	case OTRL_MSGEVENT_CONNECTION_ENDED:
	    buf = g_strdup_printf(_("%s has already closed his/her private "
			"connection to you"), context->username);
	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, _("Your message "
		    "was not sent.  Either end your private conversation, "
		    "or restart it."), 1, OTRL_NOTIFY_ERROR,
		    _("Private connection closed"), buf,
		    _("Your message was not sent.  Either close your "
		    "private connection to him, or refresh it."));
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_SETUP_ERROR:
	    if (!err) {
		err = GPG_ERR_INV_VALUE;
	    }
	    switch(gcry_err_code(err)) {
		case GPG_ERR_INV_VALUE:
		    buf = g_strdup(_("Error setting up private "
			    "conversation: Malformed message received"));
		    break;
		default:
		    buf = g_strdup_printf(_("Error setting up private "
			    "conversation: %s"), gcry_strerror(err));
		    break;
	    }

	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, buf, 1,
		    OTRL_NOTIFY_ERROR, _("OTR Error"), buf, NULL);
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_MSG_REFLECTED:
	    display_otr_message_or_notify(opdata,
		    context->accountname, context->protocol,
		    context->username,
		    _("We are receiving our own OTR messages.  "
		    "You are either trying to talk to yourself, "
		    "or someone is reflecting your messages back "
		    "at you."), 1, OTRL_NOTIFY_ERROR,
		    _("OTR Error"), _("We are receiving our own OTR messages."),
		    _("You are either trying to talk to yourself, "
		    "or someone is reflecting your messages back "
		    "at you."));
	    break;
	case OTRL_MSGEVENT_MSG_RESENT:
	    buf = g_strdup_printf(_("<b>The last message to %s was resent."
		    "</b>"), context->username);
	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, buf, 1,
		    OTRL_NOTIFY_INFO, _("Message resent"), buf, NULL);
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE:
	    buf = g_strdup_printf(_("<b>The encrypted message received from "
		    "%s is unreadable, as you are not currently communicating "
		    "privately.</b>"), context->username);
	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, buf, 1,
		    OTRL_NOTIFY_INFO, _("Unreadable message"), buf, NULL);
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_RCVDMSG_UNREADABLE:
	    buf = g_strdup_printf(_("We received an unreadable "
		    "encrypted message from %s."), context->username);
	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, buf, 1,
		    OTRL_NOTIFY_ERROR, _("OTR Error"), buf, NULL);
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_RCVDMSG_MALFORMED:
	    buf = g_strdup_printf(_("We received a malformed data "
		    "message from %s."), context->username);
	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, buf, 1,
		    OTRL_NOTIFY_ERROR, _("OTR Error"), buf, NULL);
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_LOG_HEARTBEAT_RCVD:
	    buf = g_strdup_printf(_("Heartbeat received from %s.\n"),
		    context->username);
	    log_message(opdata, buf);
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_LOG_HEARTBEAT_SENT:
	    buf = g_strdup_printf(_("Heartbeat sent to %s.\n"),
		    context->username);
	    log_message(opdata, buf);
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_RCVDMSG_GENERAL_ERR:
	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, message, 1,
		    OTRL_NOTIFY_ERROR, _("OTR Error"), message, NULL);
	    break;
	case OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED:
	    buf = g_strdup_printf(_("<b>The following message received "
		    "from %s was <i>not</i> encrypted: [</b>%s<b>]</b>"),
		    context->username, message);
	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, buf, 1,
		    OTRL_NOTIFY_INFO, _("Received unencrypted message"),
		    buf, NULL);
	    emit_msg_received(context, buf);
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED:
	    buf = g_strdup_printf(_("Unrecognized OTR message received "
		    "from %s.\n"), context->username);
	    log_message(opdata, buf);
	    g_free(buf);
	    break;
	case OTRL_MSGEVENT_RCVDMSG_FOR_OTHER_INSTANCE:
	    if (*last_msg_event == msg_event) {
		break;
	    }
	    buf = g_strdup_printf(_("%s has sent a message intended for a "
		    "different session. If you are logged in multiple times, "
		    "another session may have received the message."),
		    context->username);
	    display_otr_message_or_notify(opdata, context->accountname,
		    context->protocol, context->username, buf, 1,
		    OTRL_NOTIFY_INFO, _("Received message for a different "
		    "session"), buf, NULL);
	    g_free(buf);
	    break;
    }

    *last_msg_event = msg_event;
*/

}


void wtwOTRmessaging::OTRL_create_instag_cb(void *opdata, const char *accountname, const char *protocol)
{
	//LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));

	FILE *instagf;

	const char *instagfile = utf16Toutf8(instance->itsSettingsBroker.getInstagFileFullPath());
	if (!instagfile) {
		LOG_ERROR(L"Instag file path is nullptr");
		return;
	}

	LOG_DEBUG(L"Creating instag & save to file %s", utf8Toutf16(instagfile));

	if (0 == fopen_s(&instagf, instagfile, "w+b"))
	{
		/* Generate the instag */
		otrl_instag_generate_FILEp(instance->itsOtrlUserState, instagf, accountname, protocol);
		fclose(instagf);
	}
	else
	{
		LOG_ERROR(L"%s() Could not write private key file\n", utf8Toutf16(__FUNCTION__));
	}
}


void wtwOTRmessaging::OTRL_convert_msg_cb(void *opdata, ConnContext *context, OtrlConvertType convert_type,
											char ** dest, const char *src)
{
	LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
}


void wtwOTRmessaging::OTRL_convert_free_cb(void *opdata, ConnContext *context, char *dest)
{
	LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
}


void wtwOTRmessaging::OTRL_timer_control_cb(void *opdata, unsigned int interval)
{
	instance->stop_start_timer(interval);
}


void wtwOTRmessaging::otrl_ConnContex_created(void *data, ConnContext *context)
{
	// For now, the information that a new context has been created during fingerprint read is not exploited

	//LOG_INFO(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
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
		wtw->fnCall(WTW_TIMER_DESTROY, TO_WTW_PARAM(id), TO_WTW_PARAM(nullptr));
		otrl_timer_set = false;
	}

	if (interval > 0)
	{
		wtwTimerDef timerDef;
		timerDef.id = id;
		timerDef.sleepTime = (interval*1000 > 15000) ? 15000 : interval*1000; // sleepTime in ms
		timerDef.callback = otrl_timer_callback;
		//timerDef.flags = WTW_TIMER_FLAG_ONE_TICK; // single-shot timer - we want periodic

		if (1 == wtw->fnCall(WTW_TIMER_CREATE, TO_WTW_PARAM(&timerDef), TO_WTW_PARAM(nullptr))) {
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
	otrl_message_poll(instance->getOtrlUserState(), &instance->itsOTRLops, 0);
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
			//LOG_INFO(L"%s() timer TICK", utf8Toutf16(__FUNCTION__));
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
	LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));

	// TODO: uncomment when crash is gone

	if(OTRL_TIMER_ID == idEvent) {
	otrl_message_poll(instance->getOtrlUserState(), &itsOTRLops, 0);
	} else {
	MessageBox(NULL, L"FIXME: TimerProc - unknow timer id!", L"wtwOTRmessaging error", MB_OK);
	}

}*/


WTW_PTR wtwOTRmessaging::onBeforeMsgDisp2_cb(WTW_PARAM wParam, WTW_PARAM lParam, void* userData)
{
	wtwBmd2Struct *pBmd = (wtwBmd2Struct*)wParam;

	if (pBmd->bmdResult & BMD_FORCE_NO_DISP) {
		LOG_ERROR(L"onBeforeMsgDisp2_cb - BMD_FORCE_NO_DISP!");
		MessageBox(NULL, L"onBeforeMsgDisp2_cb - BMD_FORCE_NO_DISP!", L"wtwOTRmessaging", MB_OK);
		return -1;
	}

	wtwMessageDef *pMsg = pBmd->pMsg;

	if (!pMsg || !pMsg->msgMessage || !pMsg->contactData.id) {
		LOG_ERROR(L"onBeforeMsgDisp2_cb - invalid msg ptr!");
		MessageBox(NULL, L"onBeforeMsgDisp2_cb - invalid msg ptr!", L"wtwOTRmessaging", MB_OK);
		return -1;
	}

	MY_wtwMessageDef_STRUCT *my_struct = static_cast<MY_wtwMessageDef_STRUCT*>(pMsg->ownerData);
	if ((nullptr != my_struct) && (0 != my_struct->onBeforeMsgDisp2_replace_key))
	{
		//Logger::debug(L"BEFORE DISPLAY: ownerData nonNULL");
		unsigned int key = my_struct->onBeforeMsgDisp2_replace_key;
		if (instance->encryptedToDecryptedMsgs.end() != instance->encryptedToDecryptedMsgs.find(key))
		{
			char * decyrpted_msg = instance->encryptedToDecryptedMsgs[key];

			//size_t numOfCharsConverted;
			//const int msgBufLen = 4 * 1024;
			//wchar_t msgBuffer[msgBufLen + 1];
			//mbstowcs_s(&numOfCharsConverted, msgBuffer, msgBufLen, decyrpted_msg, msgBufLen);

			//LOG_DEBUG(L"BEFORE DISPLAY: ownerData FOUND - decrypted msg below:\n'%s'\n\n", msgBuffer);

			(pBmd->fnReplaceMsg)(utf8Toutf16(decyrpted_msg), pBmd);

			// remove the key from the map but first release the memory
			otrl_message_free(decyrpted_msg);
			instance->encryptedToDecryptedMsgs.erase(key);
		}
		else {
			LOG_ERROR(L"%s() BEFORE DISPLAY: key '%u' not found in map", key, utf8Toutf16(__FUNCTION__));
		}
	}

	return 0;
}


WTW_PTR wtwOTRmessaging::onProtocolEvent_cb(WTW_PARAM wParam, WTW_PARAM lParam, void* userData)
{
	wtwProtocolEvent *pEvent = (wtwProtocolEvent *)wParam;

	if ((WTW_PEV_RAW_DATA_RECV == pEvent->event) && (WTW_PEV_TYPE_BEFORE == pEvent->type))
	{
		/*
		wtwRawDataDef * pWtwRawDataDef = (wtwRawDataDef*) lParam;
		Logger::debug(L"onProtocolEvent_cb() : MESSAGE_RECV");
		size_t numOfCharsConverted;
		wchar_t msgBuffer[1500];
		mbstowcs_s(&numOfCharsConverted, msgBuffer, 1500, pWtwRawDataDef->pData, 1500);
		wchar_t *buffer = new wchar_t[1234];
		swprintf_s(buffer, 1234, L"RAW RECV flagi: %x\ntype: %x\n'%s'",pWtwRawDataDef->flags, pEvent->type, msgBuffer);
		//MessageBox(NULL, buffer, L"onProtocolEvent_cb : WTW_PEV_MESSAGE_RECV", MB_OK);
		Logger::debug(buffer);
		delete buffer;
		*/

	}
	else if ((WTW_PEV_MESSAGE_RECV == pEvent->event) && (WTW_PEV_TYPE_BEFORE == pEvent->type))
	{
		wtwMessageDef * pMessageDef = (wtwMessageDef*)lParam;

		#if 0
		LOG_DEBUG(L"onProtocolEvent_cb() : MESSAGE_RECV [BEFORE]");
		wchar_t *buffer = new wchar_t[1234];
		swprintf_s(buffer, 1234, L"flagi: %x\ntype: %x\n'%s'", pMessageDef->msgFlags, pEvent->type, pMessageDef->msgMessage);
		LOG_DEBUG(L"%s", buffer);
		#endif

		return instance->processReceivedMessage(pMessageDef);
	}
	else if ((WTW_PEV_MESSAGE_SEND == pEvent->event) && (WTW_PEV_TYPE_BEFORE == pEvent->type))
	{
		wtwMessageDef * pMessageDef = (wtwMessageDef*)lParam;
		bool cipher = true;
		MY_wtwMessageDef_STRUCT *my_struct = static_cast<MY_wtwMessageDef_STRUCT*>(pMessageDef->ownerData);

		if (my_struct && (false == my_struct->msg_encrypt)) {
			cipher = false;
		}

		if ((pMessageDef->msgMessage) && cipher)
		{
			if (0)
			{
				LOG_DEBUG(L"onProtocolEvent_cb() : MESSAGE_SEND BEFORE START");
				wchar_t buffer[2234];
				swprintf_s(buffer, 1234, L"flagi: %x\ntype: %x\n'%s'\n\n", pMessageDef->msgFlags, pEvent->type, pMessageDef->msgMessage);
				LOG_DEBUG(L"%s\n\n", buffer);
			}

			my_struct = (my_struct) ? my_struct : MY_wtwMessageDef_STRUCT::getInstance();
			my_struct->msg_encrypt = true;
			my_struct->old_pointer_const = pMessageDef->msgMessage;	// Keep original msg since it must be restored in AFTER event
			pMessageDef->ownerData = my_struct;

			const char *unencryptedmsg = utf16Toutf8(pMessageDef->msgMessage);
			char *newmessage = NULL;
			gcry_error_t err = otrl_message_sending(instance->getOtrlUserState(),
				&instance->itsOTRLops,
				NULL, //opdata
				instance->itsSettingsBroker.getOtrlAccountName(), //accountname
				utf16Toutf8(instance->itsSettingsBroker.getProtocolNetClass()), //protocol
				utf16Toutf8(pMessageDef->contactData.id), //username
				OTRL_INSTAG_BEST, //instance,
				unencryptedmsg,
				NULL, //OtrlTLV*
				&newmessage,
				OTRL_FRAGMENT_SEND_ALL_BUT_LAST,
				NULL, NULL, NULL);

			if (GPG_ERR_NO_ERROR == err) {
				if (0 < strnlen_s(newmessage, 2)) {
					pMessageDef->msgMessage = utf8Toutf16(newmessage);
					pMessageDef->msgFlags |= WTW_MESSAGE_FLAG_ENCRYPTED | WTW_MESSAGE_FLAG_NOARCHIVE;
					otrl_message_free(newmessage);
				}
				else {
					// empty message - do not send it
					return S_FALSE;
				}
			} else {
				//pMessageDef->msgMessage = L"";
				
				LOG_ERROR(L"Error occured while trying to cipher the message");
				instance->displayMsgInChat(pMessageDef->contactData.id, L"Failed to cipher your message - it has NOT been sent!");

				// Do not send out plain text
				return S_FALSE;
			}
		}
	}
	else if ((WTW_PEV_MESSAGE_SEND == pEvent->event) && (WTW_PEV_TYPE_AFTER == pEvent->type))
	{
		wtwMessageDef * pMessageDef = (wtwMessageDef*)lParam;
		MY_wtwMessageDef_STRUCT *my_struct = static_cast<MY_wtwMessageDef_STRUCT*>(pMessageDef->ownerData);

		if (nullptr != my_struct)
		{
			if (nullptr != my_struct->old_pointer_const)
			{
				// Do not free 'pMessageDef->msgMessage' since it is from cToWideCharString() function
				pMessageDef->msgMessage = static_cast<decltype(pMessageDef->msgMessage)>(my_struct->old_pointer_const);
				my_struct->old_pointer_const = nullptr;
			}
		} else {
			LOG_ERROR(L"%s() ownerData is nullptr", utf8Toutf16(__FUNCTION__));
			pMessageDef->msgMessage = L"Your message has been lost :(";
		}
	}
	else if ((WTW_PEV_RAW_DATA_SEND == pEvent->event) && (WTW_PEV_TYPE_BEFORE == pEvent->type))
	{
		/*
		wtwRawDataDef * pDataDef = (wtwRawDataDef*)lParam;
		LOG_DEBUG(L"onProtocolEvent_cb() : WTW_PEV_RAW_DATA_SEND BEFORE");
		LOG_DEBUG(L"dlugosc %d  flags %x\n '%s'", pDataDef->pDataLen, pDataDef->flags, utf8Toutr16(pDataDef->pData));
		*/
	}
	else {
		// for now we are interested only in some WTW protocol events - ignore the rest
		//LOG_ERROR(L"%s() event type unhandled - add more else cases", cToWideCharString(__FUNCTION__));
	}

	return S_OK;
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
		LOG_WARN(L"initializeOTRL() read fingerprints failed");
	}
}


void wtwOTRmessaging::releaseOTRL()
{
	// TODO: destroy timer if running/set

	otrl_userstate_free(itsOtrlUserState);
	itsOtrlUserState = nullptr;
}


void wtwOTRmessaging::installWtwHooks()
{
	onBeforeMsgDisp2_hook = wtw->evHook(WTW_EVENT_CHATWND_BEFORE_MSG_DISP2,
		wtwOTRmessaging::onBeforeMsgDisp2_cb, this);

	onProtocolEvent_hook = wtw->evHook(WTW_ON_PROTOCOL_EVENT,
		wtwOTRmessaging::onProtocolEvent_cb, this);
}


void wtwOTRmessaging::removeWtwHooks()
{
	wtw->evUnhook(onProtocolEvent_hook);
	wtw->evUnhook(onBeforeMsgDisp2_hook);
}


void wtwOTRmessaging::installWtwFunctions()
{
	//LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));

	//handler = wtwInstProtoFunc(wtw, PROTO_NAME, netSID, WTW_PF_MESSAGE_SEND, &funcMessageSend, 0);
}


void wtwOTRmessaging::removeWtwFunctions()
{
	//LOG_ERROR(L"%s() not implemented", utf8Toutf16(__FUNCTION__));
	// TODO: implement it :)
}


int wtwOTRmessaging::processReceivedMessage(wtwMessageDef *pWtwMessage)
{
	int ignoreMsg = 0;
	char *newMsg = NULL;
	OtrlTLV *tlvs = NULL;

	//Logger::debug(L"processReceivedMessage(): ");
	//Logger::debug(pWtwMessage->msgMessage);

	ignoreMsg = otrl_message_receiving(itsOtrlUserState, &itsOTRLops, NULL,
		itsSettingsBroker.getOtrlAccountName(), utf16Toutf8(pWtwMessage->contactData.netClass),
		utf16Toutf8(pWtwMessage->contactData.id), utf16Toutf8(pWtwMessage->msgMessage), &newMsg,
		&tlvs, NULL, NULL, NULL);

	// Check if other side has ended the secure connection
	{
		wchar_t msg[2 * 1024];
		OtrlTLV *tlv = otrl_tlv_find(tlvs, OTRL_TLV_DISCONNECTED);
		if (tlv) {
			/* Notify the user that the other side disconnected. */
			StringCbPrintfW(msg, sizeof(msg), L"Prywatna rozmowa z u¿ytkownikiem '%s' zosta³a zakoñczona", pWtwMessage->contactData.id);
			instance->displayMsgInChat(pWtwMessage->contactData.id, msg);

			ChatBroker::update_ui();
		}
		otrl_tlv_free(tlvs);
	}

	if (ignoreMsg) {
		//Logger::debug(L"processReceivedMessage() IGNORUJE MSG");
		pWtwMessage->msgFlags |= WTW_MESSAGE_FLAG_ENCRYPTED | WTW_MESSAGE_FLAG_NOARCHIVE;
		return S_FALSE;
	}
	else {
		//Logger::debug(L"NIE IGNORUJE");

		if (newMsg)
		{
			/*
			Logger::debug(L"wiadomosc po zdekodowaniu:");
			Logger::debug(utf8Toutf16(newMsg).data());
			Logger::debug(L"\n");
			*/

			pWtwMessage->msgFlags |= WTW_MESSAGE_FLAG_ENCRYPTED | WTW_MESSAGE_FLAG_NOARCHIVE;

			// Do not replace pWtwMessage->msgMessage pointer here since
			// we cannot free that pointer here!
			// The decrypted message is kept in the map "encryptedToDecryptedMsgs"
			// and the displayed message is replaced on onBeforeMsgDisp2_cb event.
			//
			// NOTE: That event also is responsible for freeing 'newMsg'!

			unsigned int key = getEncryptedToDecryptedMsgsId();
			if (key) {
				encryptedToDecryptedMsgs[key] = newMsg;
				MY_wtwMessageDef_STRUCT *my_struct = reinterpret_cast<MY_wtwMessageDef_STRUCT*>(pWtwMessage->ownerData);
				my_struct = (my_struct) ? my_struct : MY_wtwMessageDef_STRUCT::getInstance();
				my_struct->onBeforeMsgDisp2_replace_key = key;
				pWtwMessage->ownerData = my_struct;
			}
			else {
				LOG_ERROR(L"FIX ME: processReceivedMessage() - key==0 => message dropped");
				otrl_message_free(newMsg);
			}
		}
		else {
			//LOG_DEBUG(L"pusty pointer newMsg! nieszyfrowana wiadomosc ktora trzeba przekazac dalej");
		}
	}

	return S_OK;
}


unsigned int wtwOTRmessaging::getEncryptedToDecryptedMsgsId()
{
	static unsigned int key = 0;

	int attempt = 0;
	while (attempt < 32) {
		++key;

		if (instance->encryptedToDecryptedMsgs.end() == instance->encryptedToDecryptedMsgs.find(key))
			return key;

		attempt++;
	}

	Logger::critical(L"getEncryptedToDecryptedMsgsId() failed");
	return 0; // 0 means incorrect key!
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
	if (wtw->fnCall(WTW_PF_CALL_HOOKS, reinterpret_cast<WTW_PARAM>(&ev), reinterpret_cast<WTW_PARAM>(&rawData)) == 0)
	{
		ev.type = WTW_PEV_TYPE_AFTER;
		wtw->fnCall(WTW_PF_CALL_HOOKS, reinterpret_cast<WTW_PARAM>(&ev), reinterpret_cast<WTW_PARAM>(&rawData));
		return 0;
	}

	return 1;
}


int wtwOTRmessaging::sendRawMessageToContact(wtwContactDef *contactDef, const wchar_t *msg, unsigned int extraMsgFlags)
{
	//LOG_INFO(L"%s() wyslam raw msg: %s", utf8Toutf16(__FUNCTION__), msg);

	MY_wtwMessageDef_STRUCT *my_struct = MY_wtwMessageDef_STRUCT::getInstance();
	my_struct->msg_encrypt = false;

	wtwMessageDef msgStruct;
	initStruct(msgStruct);
	msgStruct.contactData = *contactDef;
	msgStruct.msgFlags = extraMsgFlags;
	msgStruct.msgMessage = msg;
	msgStruct.ownerData = my_struct;

	wchar_t _f[255] = { 0 };
//	wsprintf(_f, L"%s/%d/%s", contactDef->netClass, contactDef->netId, WTW_PF_MESSAGE_SEND);
	StringCbPrintfW(_f, sizeof(_f), L"%s/%d/%s", contactDef->netClass, contactDef->netId, WTW_PF_MESSAGE_SEND);
	wtw->fnCall(_f, (WTW_PARAM)(&msgStruct), 0);

	return 0;
}











/* WYSY£ANIE WIADOMOSCI !

wtwProtocolEvent ev;
initStruct(ev);

ev.event = outgoing ? WTW_PEV_RAW_DATA_SEND : WTW_PEV_RAW_DATA_RECV;
ev.netClass = netClass;
ev.netId = netId;

wtwRawDataDef rd;
initStruct(rd);
rd.pData = data;
rd.pDataLen = len;
rd.flags = flags;

ev.type = WTW_PEV_TYPE_BEFORE;
if(fp->fnCall(WTW_PF_CALL_HOOKS, reinterpret_cast<WTW_PARAM>(&ev), reinterpret_cast<WTW_PARAM>(&rd)) == 0)
{
ev.type = WTW_PEV_TYPE_AFTER;
fp->fnCall(WTW_PF_CALL_HOOKS, reinterpret_cast<WTW_PARAM>(&ev), reinterpret_cast<WTW_PARAM>(&rd));
return true;
}
return false;

////////////////////////////////////////////
WERSJA DRUGA WYSY£ANIA - raczej pierwsza preferowana

WTW_PTR BmdProcess(WTW_PARAM wP, WTW_PARAM wL, void*){
wtwMessageDef *msg = (wtwMessageDef *) wP;
if(!wcscmp(msg->msgMessage,L"test")){

DWORD a=msg->msgFlags;
(msg->msgFlags |= WTW_MESSAGE_FLAG_OUTGOING) &= ~WTW_MESSAGE_FLAG_INCOMING;
wchar_t _f[255] = {0};
wsprintf(_f, L"%s/%d/%s", msg->contactData.netID, msg->contactData.netSID, WTW_PF_MESSAGE_SEND);
wtw->fnCall(_f, (WTW_PARAM)msg, 0);
msg->msgFlags=a;
return BMD_FORCE_NO_STORE|BMD_FORCE_NO_DISP;
}
return BMD_OK;
}
*/




int wtwOTRmessaging::otrg_plugin_send_default_query_conv(wtwContactDef *contact)
{
	char *msg;
	int ret = 0;

	// TODO: replace with current user login or name
	msg = otrl_proto_default_query_msg("name", OTRL_POLICY_DEFAULT);

	if (msg) {
		ret = instance->sendRawMessageToContact(contact, utf8Toutf16(msg),
				WTW_MESSAGE_FLAG_OUTGOING | WTW_MESSAGE_FLAG_CHAT_MSG);
		free(msg);
	}
	else
	{
		LOG_ERROR(L"%s() otrl_proto_default_query_msg() returned nullptr!", utf8Toutf16(__FUNCTION__));
	}

	return ret;
}


int wtwOTRmessaging::otrg_finish_private_conversation(wtwContactDef *contact)
{

	otrl_message_disconnect(instance->getOtrlUserState(),
		&instance->itsOTRLops,
		NULL,
		instance->itsSettingsBroker.getOtrlAccountName(),
		utf16Toutf8(instance->itsSettingsBroker.getProtocolNetClass()),
		utf16Toutf8(contact->id),
		OTRL_INSTAG_BEST);

	wchar_t s[500];
	StringCbPrintfW(s, sizeof(s), L"Zakoñczono prywatn¹ rozmowê z u¿ytkownikiem '%s'", contact->id);
	instance->displayMsgInChat(contact->id, s);

	return 0;
}




void wtwOTRmessaging::displayMsgInChat(const wchar_t *peer, const wchar_t *msg, bool fontBold, bool tooltip)
{
	const int max_len = 2048;
	size_t len;
	bool len_ok = false;

	if (S_OK == StringCchLength(msg, max_len+1, &len)) {
		if (len < max_len) {
			len_ok = true;
		} else {
			LOG_ERROR(L"%s() string is too long %u (max 2000)", utf8Toutf16(__FUNCTION__), (unsigned)len);
		}
	}

	if (nullptr != peer && nullptr != msg && true == len_ok)
	{
		wchar_t decorated[2048 + 512];
		StringCbPrintfW(decorated, sizeof(decorated),
			L"<span %s %s> %s </span>",
			(fontBold) ? L"style=\"font-weight:bold;\"" : L"",
			(tooltip) ? L"title=\"Wiadomoœæ od wtyczki wtwOTRmessaging\"" : L"",
			msg);

		wtwMessageDef md;
		initStruct(md);
		md.contactData.id = peer;
		md.contactData.netClass = itsSettingsBroker.getProtocolNetClass();
		md.contactData.netId = itsSettingsBroker.getProtocolNetId();
		md.msgMessage = decorated;
		md.msgTime = time(NULL);
		md.msgFlags = WTW_MESSAGE_FLAG_INCOMING | WTW_MESSAGE_FLAG_CHAT_MSG | WTW_MESSAGE_FLAG_NOHTMLESC
			| WTW_MESSAGE_FLAG_NOARCHIVE;
		wtw->fnCall(WTW_CHATWND_SHOW_MESSAGE, TO_WTW_PARAM(&md), 0);
	}
}

