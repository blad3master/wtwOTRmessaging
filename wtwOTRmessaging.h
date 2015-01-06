// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the WTWOTRMESSAGING_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// WTWOTRMESSAGING_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WTWOTRMESSAGING_EXPORTS
#define WTWOTRMESSAGING_API __declspec(dllexport)
#else
#define WTWOTRMESSAGING_API __declspec(dllimport)
#endif
/* EXAMPLES
extern WTWOTRMESSAGING_API int nwtwOTRmessaging;
WTWOTRMESSAGING_API int fnwtwOTRmessaging(void);
*/

#pragma once

#include "ChatBroker.h"
#include "SettingsBroker.h"
#include "utils.h"

extern "C" {
#include "OTRL/message.h"
}

#include <map>

// use Windows Timers instead of WTW Timer API
#define USE_WINAPI_TIMERS 1

WTW_PLUGIN_API_ENTRY(WTWPLUGINFO*) queryPlugInfo(DWORD /*apiVersion*/, DWORD /*masterVersion*/);

WTW_PLUGIN_API_ENTRY(int) pluginLoad(DWORD /*callReason*/, WTWFUNCTIONS* f);

WTW_PLUGIN_API_ENTRY(int) pluginUnload(DWORD /*callReason*/);


// The struct keeps some data that help to process the message in hooks
struct MY_wtwMessageDef_STRUCT
{
private:
	static const int _instace_number = 16;
	static int _instance_index;
	static MY_wtwMessageDef_STRUCT _instances[_instace_number];

	// do not let create instances - 
	MY_wtwMessageDef_STRUCT() {};
	MY_wtwMessageDef_STRUCT(const MY_wtwMessageDef_STRUCT &) = delete;
	MY_wtwMessageDef_STRUCT(const MY_wtwMessageDef_STRUCT &&) = delete;
	MY_wtwMessageDef_STRUCT &operator =(const MY_wtwMessageDef_STRUCT &) = delete;
	MY_wtwMessageDef_STRUCT &operator =(const MY_wtwMessageDef_STRUCT &&) = delete;

public:
	// Message will be encrypted by OTRL
	bool msg_encrypt = false;

	void * old_pointer = nullptr;

	const void * old_pointer_const = nullptr;

	// Is message received from network and requires substition to decoded version?
	// value of 0 means incorrect key
	unsigned int onBeforeMsgDisp2_replace_key = 0;

	static MY_wtwMessageDef_STRUCT *getInstance() {
		if (++_instance_index >= _instace_number)
			_instance_index = 0;
		memset(&_instances[_instance_index], 0, sizeof(_instances[0]));
		return &_instances[_instance_index];
	}
};


// The struct keeps some data that help to process the OTRL events
struct MY_ConnContext_STRUCT
{
	static const int _instace_number = 16;
	static int _instance_index;
	static MY_ConnContext_STRUCT _instances[_instace_number];

	MY_ConnContext_STRUCT(const MY_ConnContext_STRUCT &) = delete;
	MY_ConnContext_STRUCT(const MY_ConnContext_STRUCT &&) = delete;
	MY_ConnContext_STRUCT &operator =(const MY_ConnContext_STRUCT &) = delete;
	MY_ConnContext_STRUCT &operator =(const MY_ConnContext_STRUCT &&) = delete;

public:
	wchar_t peer[255];
	wchar_t netClass[255];
	int netId;
	bool privateConverstationEnded;	// set to true when trying to send encrypted msg but peer has ended it

	MY_ConnContext_STRUCT() {}

	static MY_ConnContext_STRUCT *getInstance() {
		if (++_instance_index >= _instace_number)
			_instance_index = 0;
		memset(&_instances[_instance_index], 0, sizeof(_instances[0]));
		return &_instances[_instance_index];
	}
};


class wtwOTRmessaging {
	
	friend class ChatBroker;
	friend class SettingsBroker;

public:
	wtwOTRmessaging();

	~wtwOTRmessaging();

	static wtwOTRmessaging *instance;

	// TODO: get main window HWND for msgbox like purposes
	// fnCall (WTW_GET_MAIN_HWND)
	// HWND wtwMainHwnd;

public:
	inline ChatBroker & getChatBroker();

	inline SettingsBroker & getSettingsBroker();

	inline OtrlUserState getOtrlUserState();

	static int sendRawMessageToContact(wtwContactDef* contactDef, const wchar_t *msg, unsigned int extraMsgFlags);

	static int otrg_plugin_send_default_query_conv(wtwContactDef *contact);

	static int otrg_finish_private_conversation(wtwContactDef *contact);

private:
	/************/
	/* CONSTATS */
	/************/

//	static const UINT_PTR OTRL_TIMER_ID = 1;

	static const OtrlMessageAppOps itsOTRLops;

	/*************/
	/* VARIABLES */
	/*************/

	Logger logger;

	ChatBroker itsChatBroker;

	SettingsBroker itsSettingsBroker;

	// OTR Library
	OtrlUserState itsOtrlUserState = nullptr;

	// Current timer id for OTR Library
	//static UINT_PTR OTRL_timer_id;

	// Mappings of encrypted messages to their unencrypted form
	std::map<unsigned int, char*> encryptedToDecryptedMsgs;

	// WTW hooks
	HANDLE onBeforeMsgDisp2_hook;
	HANDLE onProtocolEvent_hook;
	HANDLE onChatwndBeforeMsgProc_hook;
	HANDLE onCELReceive_hook;
	HANDLE onCELBeforeSend_hook;


	bool otrl_timer_set = false;
#if USE_WINAPI_TIMERS
	UINT_PTR otrl_timer_id = 0;
#endif

	void stop_start_timer(unsigned int interval);

	/*************/
	/* CALLBACKS */
	/*************/

#if USE_WINAPI_TIMERS
	static VOID CALLBACK otrl_timer_callback(_In_  HWND hwnd, _In_  UINT uMsg, _In_  UINT_PTR idEvent, _In_  DWORD dwTime);
#else
	static WTW_PTR otrl_timer_callback(WTW_PARAM, WTW_PARAM, void*);
#endif


	// Timer timeout callback for OTR Library
//	static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

	// WTW protocol event callback
	static WTW_PTR onProtocolEvent_cb(WTW_PARAM, WTW_PARAM, void*);

	// WTW callback
	static WTW_PTR onBeforeMsgDisp2_cb(WTW_PARAM, WTW_PARAM, void*);

	// WTW callback
	static WTW_PTR onChatwndBeforeMsgProc_cb(WTW_PARAM, WTW_PARAM, void*);

	// WTW callback
	static WTW_PTR onCELReceive_cb(WTW_PARAM, WTW_PARAM, void*);

	// WTW callback
	static WTW_PTR onCELBeforeSend_cb(WTW_PARAM, WTW_PARAM, void*);

	/******************/
	/* OTRL CALLBACKS */
	/******************/

	/* Return the OTR policy for the given context. */
	static OtrlPolicy OTRL_policy_cb(void *opdata, ConnContext *context);

	/* Create a private key for the given accountname/protocol if desired. */
	static void OTRL_create_privkey_cb(void *opdata, const char *accountname, const char *protocol);

	/* Report whether you think the given user is online.  Return 1 if
	* you think he is, 0 if you think he isn't, -1 if you're not sure.
	*
	* If you return 1, messages such as heartbeats or other
	* notifications may be sent to the user, which could result in "not
	* logged in" errors if you're wrong. */
	static int OTRL_is_logged_in_cb(void *opdata, const char *accountname,
		const char *protocol, const char *recipient);

	/* Send the given IM to the given recipient from the given
	* accountname/protocol. */
	static void OTRL_inject_message_cb(void *opdata, const char *accountname,
		const char *protocol, const char *recipient, const char *message);

	/* When the list of ConnContexts changes (including a change in
	* state), this is called so the UI can be updated. */
	static void OTRL_update_context_list_cb(void *opdata);

	/* A new fingerprint for the given user has been received. */
	static void OTRL_new_fingerprint_cb(void *opdata, OtrlUserState us,
		const char *accountname, const char *protocol,
		const char *username, unsigned char fingerprint[20]);

	/* The list of known fingerprints has changed.  Write them to disk. */
	static void OTRL_write_fingerprints_cb(void *opdata);

	/* A ConnContext has entered a secure state. */
	static void OTRL_gone_secure_cb(void *opdata, ConnContext *context);

	/* We have completed an authentication, using the D-H keys we
	* already knew.  is_reply indicates whether we initiated the AKE. */
	static void OTRL_still_secure_cb(void *opdata, ConnContext *context, int is_reply);

	/* Find the maximum message size supported by this protocol. */
	static int OTRL_max_message_size_cb(void *opdata, ConnContext *context);

	/* We received a request from the buddy to use the current "extra"
	* symmetric key.  The key will be passed in symkey, of length
	* OTRL_EXTRAKEY_BYTES.  The requested use, as well as use-specific
	* data will be passed so that the applications can communicate other
	* information (some id for the data transfer, for example). */
	static void OTRL_received_symkey_cb(void *opdata, ConnContext *context,
		unsigned int use, const unsigned char *usedata,
		size_t usedatalen, const unsigned char *symkey);

	/* Return a string according to the error event. This string will then
	* be concatenated to an OTR header to produce an OTR protocol error
	* message. The following are the possible error events:
	* - OTRL_ERRCODE_ENCRYPTION_ERROR
	* 		occured while encrypting a message
	* - OTRL_ERRCODE_MSG_NOT_IN_PRIVATE
	* 		sent encrypted message to somebody who is not in
	* 		a mutual OTR session
	* - OTRL_ERRCODE_MSG_UNREADABLE
	*		sent an unreadable encrypted message
	* - OTRL_ERRCODE_MSG_MALFORMED
	* 		message sent is malformed */
	static const char *OTRL_otr_error_message_cb(void *opdata, ConnContext *context,
		OtrlErrorCode err_code);

	/* Deallocate a string returned by otr_error_message */
	static void OTRL_otr_error_message_free_cb(void *opdata, const char *err_msg);

	/* Return a string that will be prefixed to any resent message. If this
	* function is not provided by the application then the default prefix,
	* "[resent]", will be used.
	* */
	static const char *OTRL_resent_msg_prefix_cb(void *opdata, ConnContext *context);

	/* Deallocate a string returned by resent_msg_prefix */
	static void OTRL_resent_msg_prefix_free_cb(void *opdata, const char *prefix);

	/* Update the authentication UI with respect to SMP events
	* These are the possible events:
	* - OTRL_SMPEVENT_ASK_FOR_SECRET
	*      prompt the user to enter a shared secret. The sender application
	*      should call otrl_message_initiate_smp, passing NULL as the question.
	*      When the receiver application resumes the SM protocol by calling
	*      otrl_message_respond_smp with the secret answer.
	* - OTRL_SMPEVENT_ASK_FOR_ANSWER
	*      (same as OTRL_SMPEVENT_ASK_FOR_SECRET but sender calls
	*      otrl_message_initiate_smp_q instead)
	* - OTRL_SMPEVENT_CHEATED
	*      abort the current auth and update the auth progress dialog
	*      with progress_percent. otrl_message_abort_smp should be called to
	*      stop the SM protocol.
	* - OTRL_SMPEVENT_INPROGRESS 	and
	*   OTRL_SMPEVENT_SUCCESS 		and
	*   OTRL_SMPEVENT_FAILURE    	and
	*   OTRL_SMPEVENT_ABORT
	*      update the auth progress dialog with progress_percent
	* - OTRL_SMPEVENT_ERROR
	*      (same as OTRL_SMPEVENT_CHEATED)
	* */
	static void OTRL_handle_smp_event_cb(void *opdata, OtrlSMPEvent smp_event,
		ConnContext *context, unsigned short progress_percent,
		char *question);

	/* Handle and send the appropriate message(s) to the sender/recipient
	* depending on the message events. All the events only require an opdata,
	* the event, and the context. The message and err will be NULL except for
	* some events (see below). The possible events are:
	* - OTRL_MSGEVENT_ENCRYPTION_REQUIRED
	*      Our policy requires encryption but we are trying to send
	*      an unencrypted message out.
	* - OTRL_MSGEVENT_ENCRYPTION_ERROR
	*      An error occured while encrypting a message and the message
	*      was not sent.
	* - OTRL_MSGEVENT_CONNECTION_ENDED
	*      Message has not been sent because our buddy has ended the
	*      private conversation. We should either close the connection,
	*      or refresh it.
	* - OTRL_MSGEVENT_SETUP_ERROR
	*      A private conversation could not be set up. A gcry_error_t
	*      will be passed.
	* - OTRL_MSGEVENT_MSG_REFLECTED
	*      Received our own OTR messages.
	* - OTRL_MSGEVENT_MSG_RESENT
	*      The previous message was resent.
	* - OTRL_MSGEVENT_RCVDMSG_NOT_IN_PRIVATE
	*      Received an encrypted message but cannot read
	*      it because no private connection is established yet.
	* - OTRL_MSGEVENT_RCVDMSG_UNREADABLE
	*      Cannot read the received message.
	* - OTRL_MSGEVENT_RCVDMSG_MALFORMED
	*      The message received contains malformed data.
	* - OTRL_MSGEVENT_LOG_HEARTBEAT_RCVD
	*      Received a heartbeat.
	* - OTRL_MSGEVENT_LOG_HEARTBEAT_SENT
	*      Sent a heartbeat.
	* - OTRL_MSGEVENT_RCVDMSG_GENERAL_ERR
	*      Received a general OTR error. The argument 'message' will
	*      also be passed and it will contain the OTR error message.
	* - OTRL_MSGEVENT_RCVDMSG_UNENCRYPTED
	*      Received an unencrypted message. The argument 'smessage' will
	*      also be passed and it will contain the plaintext message.
	* - OTRL_MSGEVENT_RCVDMSG_UNRECOGNIZED
	*      Cannot recognize the type of OTR message received.
	* - OTRL_MSGEVENT_RCVDMSG_FOR_OTHER_INSTANCE
	*      Received and discarded a message intended for another instance. */
	static void OTRL_handle_msg_event_cb(void *opdata, OtrlMessageEvent msg_event,
		ConnContext *context, const char *message,
		gcry_error_t err);

	/* Create a instance tag for the given accountname/protocol if
	* desired. */
	static void OTRL_create_instag_cb(void *opdata, const char *accountname,
		const char *protocol);

	/* Called immediately before a data message is encrypted, and after a data
	* message is decrypted. The OtrlConvertType parameter has the value
	* OTRL_CONVERT_SENDING or OTRL_CONVERT_RECEIVING to differentiate these
	* cases. */
	static void OTRL_convert_msg_cb(void *opdata, ConnContext *context,
		OtrlConvertType convert_type, char ** dest, const char *src);

	/* Deallocate a string returned by convert_msg. */
	static void OTRL_convert_free_cb(void *opdata, ConnContext *context, char *dest);

	/* When timer_control is called, turn off any existing periodic
	* timer.
	*
	* Additionally, if interval > 0, set a new periodic timer
	* to go off every interval seconds.  When that timer fires, you
	* must call otrl_message_poll(userstate, uiops, uiopdata); from the
	* main libotr thread.
	*
	* The timing does not have to be exact; this timer is used to
	* provide forward secrecy by cleaning up stale private state that
	* may otherwise stick around in memory.  Note that the
	* timer_control callback may be invoked from otrl_message_poll
	* itself, possibly to indicate that interval == 0 (that is, that
	* there's no more periodic work to be done at this time).
	*
	* If you set this callback to NULL, then you must ensure that your
	* application calls otrl_message_poll(userstate, uiops, uiopdata);
	* from the main libotr thread every definterval seconds (where
	* definterval can be obtained by calling
	* definterval = otrl_message_poll_get_default_interval(userstate);
	* right after creating the userstate).  The advantage of
	* implementing the timer_control callback is that the timer can be
	* turned on by libotr only when it's needed.
	*
	* It is not a problem (except for a minor performance hit) to call
	* otrl_message_poll more often than requested, whether
	* timer_control is implemented or not.
	*
	* If you fail to implement the timer_control callback, and also
	* fail to periodically call otrl_message_poll, then you open your
	* users to a possible forward secrecy violation: an attacker that
	* compromises the user's computer may be able to decrypt a handful
	* of long-past messages (the first messages of an OTR
	* conversation).
	*/
	static void OTRL_timer_control_cb(void *opdata, unsigned int interval);

	/////////////////////////////////////////////
	// OTRL - other callbacks (not from structure)

	static void otrl_ConnContex_created(void *data, ConnContext *context);

	/**********************/
	/* INTERNAL FUNCTIONS */
	/**********************/

	void setInstancePointer();

	void unsetInstancePointer();

	void initializeOTRL();

	void releaseOTRL();

	void installWtwHooks();

	void removeWtwHooks();

	void installWtwFunctions();

	void removeWtwFunctions();

	int processReceivedMessage(wtwMessageDef *pWtwMessage);

	// Generator of keys for encryptedToDecryptedMsgs map
	static unsigned int getEncryptedToDecryptedMsgsId();

	static int sendRawMessageToNetwork(const char* msg);

	void displayMsgInChat(const wchar_t *peer, const wchar_t *netClass, int netId,
		const wchar_t *msg, bool fontBold = true, bool tooltip = true);

	// this calls above displayMsgInChat()
	void displayMsgInChat(const MY_ConnContext_STRUCT* wtwContact,
		const wchar_t *msg, bool fontBold = true, bool tooltip = true);

	// return true if plugin works with give protocol
	static bool isNetClassSupported(const wchar_t* netClass);
};

ChatBroker & wtwOTRmessaging::getChatBroker() {
	return itsChatBroker;
}

SettingsBroker & wtwOTRmessaging::getSettingsBroker() {
	return itsSettingsBroker;
}

OtrlUserState wtwOTRmessaging::getOtrlUserState() {
	return itsOtrlUserState;
}

