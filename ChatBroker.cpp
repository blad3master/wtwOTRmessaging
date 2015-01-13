#include "StdAfx.h"
#include "ChatBroker.h"
#include "wtwapi/wtwChatHeaderBar.h"
#include "wtwOTRmessaging.h"

extern "C" {
#include "OTRL/proto.h"
}

#include<algorithm>
#include<sstream>
#include <Strsafe.h> // StringCchPrintW()

#pragma warning(disable: 4995) /*  'wsprintfA': name was marked as #pragma deprecated */
#include <atlwin.h>
#include "resource.h"
#pragma warning(default: 4995)

extern "C" {
	#include "OTRL/privkey.h"
}

		/*
		#include <windows.h>
		#include <atlbase.h>
		#include <atldef.h>
		#include <atlwin.h>
		#include <atlcom.h>
		#include <atlapp.h>
		*/
	//	#include <atlapp.h>

const wchar_t * const ChatBroker::GraphId_notPrivate	= L"wtwOTRmessaging/notPrivate";
const wchar_t * const ChatBroker::GraphId_private		= L"wtwOTRmessaging/private";
const wchar_t * const ChatBroker::GraphId_unverified	= L"wtwOTRmessaging/unverified";
const wchar_t * const ChatBroker::GraphId_help			= L"wtwOTRmessaging/help";
const wchar_t * const ChatBroker::GraphId_identity		= L"wtwOTRmessaging/identity";
const wchar_t * const ChatBroker::GraphId_refresh		= L"wtwOTRmessaging/refresh";

std::map<std::wstring, void*> ChatBroker::chatWndList;


ChatBroker::ChatBroker(void)
{
	loadGraphics();

	// When chat window is shown, execute handler
	onChatWndCreateHook = wtwPf->evHook(WTW_EVENT_ON_CHATWND_CREATE, ChatBroker::onChatWndCreate, this);
	onChatWndDestroyHook = wtwPf->evHook(WTW_EVENT_ON_CHATWND_DESTROY, ChatBroker::onChatWndDestroy, this);
	onChatWndShowHook = wtwPf->evHook(WTW_EVENT_ON_CHATWND_SHOW, ChatBroker::onChatWndShow, this);
	onChatWndMtcChangeHook = wtwPf->evHook(WTW_EVENT_ON_CHATWND_MCT_CHANGE, ChatBroker::onChatWndMetacontactChange, this);
}


ChatBroker::~ChatBroker(void)
{
	wtwPf->evUnhook(onChatWndMtcChangeHook);
	wtwPf->evUnhook(onChatWndShowHook);
	wtwPf->evUnhook(onChatWndDestroyHook);
	wtwPf->evUnhook(onChatWndCreateHook);
}


std::wstring ChatBroker::makeKey(const wchar_t *id, const wchar_t *netClass, int netId)
{
	wchar_t buf[512];
	StringCbPrintfW(buf, sizeof(buf), L"%s/%s/%d", id, netClass, netId);
	return std::wstring(buf);
}


std::wstring ChatBroker::makeKey(const wtwContactDef *contact)
{
	return makeKey(contact->id, contact->netClass, contact->netId);
}

bool ChatBroker::wtwContactFromKey(const std::wstring &key, wchar_t *id, wchar_t *netClass, int *netId, size_t strLenBytes)
{
	std::vector<std::wstring> items;
	std::wstringstream ss(key);
	std::wstring item;
	while (std::getline(ss, item, L'/')) {
		items.push_back(item);
	}

	ASSERT(3 == items.size());
	ASSERT(id);
	ASSERT(netClass);
	ASSERT(netId);
	
	if (id && netClass && netId)
	{
		StringCbPrintfW(id, strLenBytes, L"%s", items[0].c_str());
		StringCbPrintfW(netClass, strLenBytes, L"%s", items[1].c_str());
		*netId = WtwOtrContext::netIdFromAccountname(utf16Toutf8(items[2].c_str()));
		return true;
	}
	else {
		ASSERT(0);
	}

	return false;
}


WTW_PTR ChatBroker::onChatWndCreate(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr)
{
	// wP - wtwContactDef[], lP - * wtwChatInfo
	wtwContactDef *p_wtwContactDef = reinterpret_cast<wtwContactDef*>(wParam);
	wtwChatWindowInfo *p_wtwChatWindowInfo = reinterpret_cast<wtwChatWindowInfo*>(lParam);

	for (int i = 0; i < p_wtwChatWindowInfo->iContacts; ++i)
	{
		const std::wstring &key = makeKey(p_wtwContactDef[i].id, p_wtwContactDef[i].netClass, p_wtwContactDef[i].netId);
		if (chatWndList.find(key) != chatWndList.end()) {
			LOG_ERROR(L"%s() wtwContactDef pointer already in the window list!", __FUNCTIONW__);
		}
		chatWndList[key] = p_wtwChatWindowInfo->pWnd;
	}

	return 0;
}


WTW_PTR ChatBroker::onChatWndDestroy(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr)
{
	// wP - wtwContactDef[], lP - * wtwChatInfo
	wtwContactDef *p_wtwContactDef = reinterpret_cast<wtwContactDef*>(wParam);
	wtwChatWindowInfo *p_wtwChatWindowInfo = reinterpret_cast<wtwChatWindowInfo*>(lParam);

	for (int i = 0; i < p_wtwChatWindowInfo->iContacts; ++i)
	{
		const std::wstring &key = makeKey(p_wtwContactDef[i].id, p_wtwContactDef[i].netClass, p_wtwContactDef[i].netId);
		auto it = chatWndList.find(key);
		if (it != chatWndList.end()) {
			chatWndList.erase(it);
		}
		else {
			LOG_ERROR(L"%s() wtwContactDef pointer not in window list!", __FUNCTIONW__);
		}
	}

	return 0;
}


WTW_PTR ChatBroker::onChatWndShow(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr)
{
	// wP - wtwContactDef[], lP - * wtwChatInfo
	wtwContactDef *p_wtwContactDef = reinterpret_cast<wtwContactDef*>(wParam);
	wtwChatWindowInfo *p_wtwChatWindowInfo = reinterpret_cast<wtwChatWindowInfo*>(lParam);

	LOG_TRACE(L"%s() %s/%s/%d", __FUNCTIONW__,
		p_wtwContactDef[0].id, p_wtwContactDef[0].netClass, p_wtwContactDef[0].netId);

	update_ui(&p_wtwContactDef[0]); // 0 is active contact

	return 0;
}


WTW_PTR ChatBroker::onChatWndMetacontactChange(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr)
{
	wtwContactDef *p_wtwContactDef = reinterpret_cast<wtwContactDef*>(wParam);
	wtwChatWindowInfo *p_wtwChatWindowInfo = reinterpret_cast<wtwChatWindowInfo*>(lParam);

	LOG_TRACE(L"%s() %s/%s/%d", __FUNCTIONW__,
		p_wtwContactDef[0].id, p_wtwContactDef[0].netClass, p_wtwContactDef[0].netId);
	
	update_ui(&p_wtwContactDef[0]); // 0 is active contact

	return 0;
}


WTW_PTR ChatBroker::on_WtwOtrmessaging_btn_clicked(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr)
{
	wtwCommandCallback *p_callback = reinterpret_cast<wtwCommandCallback*>(wParam);
	wtwContactDef *activeContact = &p_callback->contactInfo[p_callback->selectedContact]; // needed for metacontacts
	int enabled = reinterpret_cast<int>(ptr);

	HMENU menu = CreatePopupMenu();
	InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, 10, L"Pomoc");
	setMenuItemIcon(menu, 0, GraphId_help);
	InsertMenu(menu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
	if (enabled & POPUP_ENABLE::REAUTHENTICATE) {
		InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, 5, L"Ponownie potwierdŸ to¿samoœæ");
		setMenuItemIcon(menu, 0, GraphId_identity);
	}
	if (enabled & POPUP_ENABLE::AUTHENTICATE) {
		InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, 4, L"PotwierdŸ to¿samoœæ");
		setMenuItemIcon(menu, 0, GraphId_identity);
	}
	if (enabled & POPUP_ENABLE::FINISH)
		InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, 3, L"Zakoñcz prywatn¹ rozmowê");
	if (enabled & POPUP_ENABLE::REFRESH) {
		InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, 2, L"Odœwie¿ prywatn¹ rozmowê");
		setMenuItemIcon(menu, 0, GraphId_refresh);
	}
	if (enabled & POPUP_ENABLE::START) {
		InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, 1, L"Rozpocznij prywatn¹ rozmowê");
	}
	
	BOOL ret = TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_RETURNCMD, p_callback->pt.x, p_callback->pt.y,
		0, p_callback->hWnd, NULL);
	switch (ret) {
		wchar_t s[500];
	case 2:
		StringCbPrintfW(s, sizeof(s), L"Próba odœwie¿enia prywatnej rozmowy z u¿ytkownikiem '%s'",
			activeContact->id);
		wtwOTRmessaging::instance->displayMsgInChat(activeContact->id,
			activeContact->netClass, activeContact->netId, s);
	case 1:
		initializePrivateConversation(activeContact);
		break;
	case 3:
		finishPrivateConversation(activeContact);
		break;
	case 4:
	case 5:
		authenticatePeer(activeContact);
		break;
	case 10:
		ShellExecute(NULL, L"open", L"https://otr.cypherpunks.ca/help/4.0.0/levels.php?lang=pl", NULL, NULL, SW_SHOWNORMAL);
		break;
	default:
		//MessageBox(NULL, L"Default", L"MENU", MB_OK);
		break;
	}

	return 0;
}


int ChatBroker::initializePrivateConversation(wtwContactDef *contact)
{
	if (SettingsBroker::OTRL_POLICY::NEVER != SettingsBroker::settingsBrokerInstance->getOtrlPolicy())
	{
		return wtwOTRmessaging::otrg_plugin_send_default_query_conv(contact);
	}
	else
	{
		MessageBox(NULL, L"Ustawiona politykê szyfrowania nie pozwala na szyfrowanie wiadomoœci!",
			L"wtwOTRmessaging", MB_OK);
	}
	return 0;
}

int ChatBroker::finishPrivateConversation(wtwContactDef *contact)
{
	return wtwOTRmessaging::otrg_finish_private_conversation(contact);
}


void ChatBroker::update_ui(const wtwContactDef * const activeContact)
{
	ConnContext *context = nullptr;
	
	int i = 0;
	for (auto it : chatWndList)
	{
		wchar_t *msgstate_str = L"wtwOTRmessaging";
		wchar_t *msgstate_tooltip = L"wtwOTRmessaging";
		int popup = POPUP_ENABLE::START;
		const wchar_t *graph_id = nullptr;
		bool match = false;

		//LOG_INFO(L"%s() [%d] id %s    ptr 0x%p", __FUNCTIONW__, ++i, it.first.data(), it.second);

		// update only given contact if provided
		if ((nullptr != activeContact) && (makeKey(activeContact) != it.first))	{
			continue;
		}

		for (context = wtwOTRmessaging::instance->getOtrlUserState()->context_root;
			context != nullptr; context = context->next)
		{
			// skip child context
			if (context->m_context != context) continue;

			//fingerprint = context->fingerprint_root.next;
			/*
			LOG_DEBUG(L"%s()\n"
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
				(int)context->otr_offer			);
			*/

			const std::wstring &key = makeKey(utf8Toutf16(context->username),
				utf8Toutf16(context->protocol), WtwOtrContext::netIdFromAccountname(context->accountname));

			if (it.first == key) {
				//LOG_INFO(L"%s() ##### setting new info Btn #####", __FUNCTIONW__);
				match = true;
				break;
			}
		}

		if ((nullptr != context) && (true == match))
		{
			if (nullptr != context->recent_child) {
				context = context->recent_child;
			}
			else {
				LOG_WARN(L"%s() context->recent_child is nullptr!", __FUNCTIONW__);
			}

			switch (context->msgstate) {
			case OTRL_MSGSTATE_PLAINTEXT:
				msgstate_str = L"Nieprywatna";
				msgstate_tooltip = L"Rozmowa nie jest bezpieczna";
				popup = POPUP_ENABLE::START;
				graph_id = GraphId_notPrivate;
				break;
			case OTRL_MSGSTATE_ENCRYPTED:
				if (otrl_context_is_fingerprint_trusted(context->active_fingerprint)) {
					msgstate_str = L"Prywatna";
					msgstate_tooltip = L"Rozmowa jest bezpieczna";
					popup = POPUP_ENABLE::REFRESH | POPUP_ENABLE::FINISH | POPUP_ENABLE::REAUTHENTICATE;
					graph_id = GraphId_private;
				}
				else {
					msgstate_str = L"Niezweryfikowana";
					msgstate_tooltip = L"Rozmowa mo¿e nie byæ bezpieczna\n(wymaga potwierdzenia klucza drugiej strony)";
					popup = POPUP_ENABLE::REFRESH | POPUP_ENABLE::FINISH | POPUP_ENABLE::AUTHENTICATE;
					graph_id = GraphId_unverified;
				}
				break;
			case OTRL_MSGSTATE_FINISHED:
				msgstate_str = L"Zakoñczona";
				msgstate_tooltip = L"Prywatna rozmowa zakoñczona (nie jest ju¿ bezpieczna)";
				popup = POPUP_ENABLE::START | POPUP_ENABLE::FINISH;
				graph_id = GraphId_notPrivate;
				break;
			default:
				msgstate_str = L"Status nieznany";
				msgstate_tooltip = L"";
			}

			/*
			wchar_t state[512];
			wchar_t *offer_str = nullptr;

			switch (context->otr_offer) {
			case context::OFFER_NOT: offer_str = L"OFR_NOT"; break;
			case context::OFFER_SENT: offer_str = L"OFR_SEN"; break;
			case context::OFFER_REJECTED: offer_str = L"OFR_REJ"; break;
			case context::OFFER_ACCEPTED: offer_str = L"OFR_ACC"; break;
			default: offer_str = L"OFFER_unknown";
			}

			StringCbPrintfW(state, sizeof(state), L"%s:%s:pv %u :authst %d :ssil %d",
				msgstate_str, offer_str, context->protocol_version, context->auth.authstate,
				context->auth.secure_session_id_len);
			*/
		}
		else
		{
			// no context/match so assume default values for buttons
			msgstate_str = L"Nieprywatna";
			msgstate_tooltip = L"Rozmowa nie jest bezpieczna";
			popup = POPUP_ENABLE::START;
			graph_id = GraphId_notPrivate;
		}

		// make sure it is selected contact in case of metacontacts
		// NOTE: this check MUST be ommited when activeContact is provided
		if (nullptr == activeContact)
		{
			// WTW API functions to get active window/chat tab seems not to work as expected - do not use it now
			// return  immediately here for now - maybe fix in the future?
			return;

			const int maxLen = 300;
			wchar_t buf_id[maxLen];
			wchar_t buf_netClass[maxLen];
			wtwContactDef contact;
			contact.id = buf_id;
			contact.netClass = buf_netClass;
			if (wtwContactFromKey(it.first, buf_id, buf_netClass, &contact.netId, maxLen))
			{
				wtwChatWindowAttributes attr;
				attr.pContactData = &contact;
				//attr.flags = WTW_WA_FLAG_IS_WINDOW_VISIBLE;
				if (S_OK != wtwPf->fnCall(WTW_FUNCT_GET_CHATWND_ATTRIBUTES, reinterpret_cast<WTW_PARAM>(&attr), 0))
				{
					memset(&contact, 0, sizeof(contact));
					attr.flags = WTW_WA_FLAG_GET_ACTIVE_WINDOW_INFO;
					attr.flags = WTW_WA_FLAG_GET_VISIBLE_WINDOW_INFO;
					if (S_OK != wtwPf->fnCall(WTW_FUNCT_GET_CHATWND_ATTRIBUTES, reinterpret_cast<WTW_PARAM>(&attr), 0))
					{
						if (makeKey(&contact) != it.first) {
							continue; // this contact is not active - do not update
						}
					}
				}
			}
			else
			{
				// failed to get contact from key - return immediately
				return;
			}
		}

		wchar_t ITEM_ID[] = L"wtwOTRmessaging_button";

		wtwCommandEntry ce;
		initStruct(ce);
		ce.pWnd = it.second;
		ce.itemId = ITEM_ID;
		wtwPf->fnCall(WTW_CCB_FUNCT_DEL, reinterpret_cast<WTW_PARAM>(&ce), NULL);

		initStruct(ce);
		ce.pWnd = it.second;
		ce.hInstance = hInstance;
		ce.itemId = ITEM_ID;
		ce.callback = ChatBroker::on_WtwOtrmessaging_btn_clicked;
		ce.cbData = reinterpret_cast<void*>(popup);
		ce.caption = msgstate_str;
		ce.toolTip = msgstate_tooltip;
		ce.itemFlags = CHB_FLAG_CHECKED | CHB_FLAG_CHANGEICON;
		ce.graphId = graph_id;

		wtwPf->fnCall(WTW_CCB_FUNCT_ADD, reinterpret_cast<WTW_PARAM>(&ce), NULL);

		break; // function has already updated window/tab so no sense to continue other items
	}
}



void ChatBroker::authenticatePeer(wtwContactDef *peer)
{
	// How to create dialog box in a quick way
	// INT_PTR dlgRes = DialogBox(hInst, MAKEINTRESOURCE(IDD_KEYDLG), 0, keyDlgProc);
	// https://github.com/mmuszkow/wtwCrypto/blob/master/KeyDlg.cpp

	class AuthDlg : public CSimpleDialog<IDD_DIALOG_AUTH> {
	public:

		CString my_name, my_fingerprint;
		CString peer_name, peer_fingerprint;
		int approval = -1; // -1(none selected), 0(no/invalid), 1(yes/correct)

	private:
		LRESULT OnInitDialog(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _Out_ BOOL& bHandled)
		{
			auto ret = CSimpleDialog<IDD_DIALOG_AUTH>::OnInitDialog(uMsg, wParam, lParam, bHandled);
			wchar_t buf[200];
			StringCbPrintfW(buf, sizeof(buf), L"Potwierdzenie to¿samoœci '%s'", peer_name);
			SetDlgItemTextW(IDC_AUTH_PEER, buf);

			// Set font for static text items
			CFont font;
			font.CreatePointFont(120, L"Arial", NULL); // 18 point font
			GetDlgItem(IDC_AUTH_PEER).SetFont(font);

			// Select first authentication method
			CWindow combo(GetDlgItem(IDC_COMBO_AUTH_METHOD));
			combo.SendMessage(CB_SETCURSEL, 0, 0);

			// Select approval
			CWindow comboAuth(GetDlgItem(IDC_COMBO_AUTH_APPROVAL));
			comboAuth.SendMessage(CB_SETCURSEL, approval, 0);

			// My key info
			StringCbPrintfW(buf, sizeof(buf), L"Odcisk mojego klucza '%s'", my_name);
			SetDlgItemTextW(IDC_MY_NAME, buf);
			SetDlgItemTextW(IDC_EDIT_MY_FINGERPRINT, my_fingerprint);

			// Peer key info
			StringCbPrintfW(buf, sizeof(buf), L"Odcisk klucza '%s'", peer_name);
			SetDlgItemTextW(IDC_PEER_NAME, buf);
			SetDlgItemTextW(IDC_EDIT_PEER_FINGERPRINT, peer_fingerprint);

			return ret;
		};

		LRESULT OnOK(WORD a, UINT b, HWND c, BOOL &d)
		{
			CWindow combo(GetDlgItem(IDC_COMBO_AUTH_APPROVAL));
			LRESULT sel = combo.SendMessage(CB_GETCURSEL, 0, 0);
			
			// let OK work only if no/yes selected
			if ((0 == sel) || (1 == sel))
			{
				approval = static_cast<int>(sel);
				return CSimpleDialog<IDD_DIALOG_AUTH>::OnCloseCmd(a, b, c, d);
			}

			return 0;
		}

		BEGIN_MSG_MAP(AuthDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK);
		CHAIN_MSG_MAP(CSimpleDialog<IDD_DIALOG_AUTH>)
		END_MSG_MAP()
	};

	char accountname[20];
	WtwOtrContext::accountnameFromNetId(accountname, sizeof(accountname), peer->netId);

	ConnContext *context = otrl_context_find(wtwOTRmessaging::instance->itsOtrlUserState,
							utf16Toutf8(peer->id), accountname,
							utf16Toutf8(peer->netClass),
							OTRL_INSTAG_BEST, 0, nullptr, wtwOTRmessaging::otrl_ConnContex_created,	nullptr );

	if (nullptr == context || nullptr == context->active_fingerprint || nullptr == context->active_fingerprint->fingerprint)
	{
		LOG_CRITICAL(L"%s context ptr or its field is nullptr", __FUNCTIONW__);
		return;
	}

	char my_fp[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
	otrl_privkey_fingerprint(wtwOTRmessaging::instance->itsOtrlUserState, my_fp,
		accountname, utf16Toutf8(peer->netClass));

	char peer_fp[OTRL_PRIVKEY_FPRINT_HUMAN_LEN];
	otrl_privkey_hash_to_human(peer_fp, context->active_fingerprint->fingerprint);

	AuthDlg dlg;
	dlg.my_name = accountname;
	dlg.my_fingerprint = my_fp;
	dlg.peer_name = peer->id;
	dlg.peer_fingerprint = peer_fp;
	dlg.approval = otrl_context_is_fingerprint_trusted(context->active_fingerprint) ? 1 : 0;

	INT_PTR res = dlg.DoModal();
	switch (res) {
	case IDOK:
		{
			bool changed = false;

			if (0 == dlg.approval) {
				if (otrl_context_is_fingerprint_trusted(context->active_fingerprint)) {
					changed = true;
					otrl_context_set_trust(context->active_fingerprint, nullptr);
					wtwOTRmessaging::instance->displayMsgInChat(peer->id, peer->netClass, peer->netId,
						L"Status aktualnej rozmowy: "
						L"<a href=\"https://otr.cypherpunks.ca/help/4.0.0/levels.php?lang=pl\">Niezweryfikowana</a>");
				}
			}
			else if (1 == dlg.approval) {
				if (! otrl_context_is_fingerprint_trusted(context->active_fingerprint)) {
					changed = true;
					otrl_context_set_trust(context->active_fingerprint, "trusted");
					wtwOTRmessaging::instance->displayMsgInChat(peer->id, peer->netClass, peer->netId,
						L"Status aktualnej rozmowy: "
						L"<a href=\"https://otr.cypherpunks.ca/help/4.0.0/levels.php?lang=pl\">Prywatna</a>");
				}
			}

			// common actions
			if (changed) {
				// refresh ui
				wtwOTRmessaging::OTRL_update_context_list_cb(nullptr);

				// Below should be executed in wtwOTRmessaging::OTRL_update_context_list_cb()
				{
						// but it is not since we cannot get current window/chat active so refresh by (re)opening the chat
						// open window so that button gets refreshed
						wtwPf->fnCall(WTW_FUNCT_OPEN_CHAT_WINDOW, reinterpret_cast<WTW_PARAM>(peer), 0);
					//ChatBroker::update_ui(nullptr);
				}

				wtwOTRmessaging::OTRL_write_fingerprints_cb(nullptr);
			}
		}
		break;
	case IDCANCEL:
		break;
	default:
		LOG_INFO(L"%s() Invalid dlg->doModal() result %d", __FUNCTIONW__, res);
		break;
	}
}



EXTERN_C IMAGE_DOS_HEADER __ImageBase;
//#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
void ChatBroker::loadGraphics()
{
	const HINSTANCE dllInst = ((HINSTANCE)&__ImageBase);

	static bool loadOnce = true;
	if (loadOnce)
	{
		loadOnce = false;
		std::vector < std::pair<const wchar_t*, const wchar_t*> > graphId2resourceId;
		graphId2resourceId.push_back(std::make_pair(GraphId_notPrivate,		MAKEINTRESOURCE(IDB_PNG1)));
		graphId2resourceId.push_back(std::make_pair(GraphId_private,		MAKEINTRESOURCE(IDB_PNG2)));
		graphId2resourceId.push_back(std::make_pair(GraphId_unverified,		MAKEINTRESOURCE(IDB_PNG3)));
		graphId2resourceId.push_back(std::make_pair(GraphId_help,			MAKEINTRESOURCE(IDB_PNG4)));
		graphId2resourceId.push_back(std::make_pair(GraphId_identity,		MAKEINTRESOURCE(IDB_PNG5)));
		graphId2resourceId.push_back(std::make_pair(GraphId_refresh,		MAKEINTRESOURCE(IDB_PNG6)));


		for (const auto it : graphId2resourceId)
		{
			wtwGraphics wg;
			wg.graphId = it.first;
			wg.resourceId = it.second;
			wg.hInst = hInstance;
			wg.hInst = dllInst;
			wg.imageType = 0; // png
			if (1 != wtwPf->fnCall(WTW_GRAPH_LOAD, (WTW_PARAM)&wg, 0))
			{
				LOG_ERROR(L"%s() failed to load graphics (%s)", __FUNCTIONW__, it.first);
			}
		}
	}
}


void ChatBroker::setMenuItemIcon(HMENU menu, UINT itemPos, const wchar_t *graphId)
{
#if 0
	HBITMAP bitmap = (HBITMAP)LoadImage(hInstance, L"C:\\icon.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
#elif 1
	wtwGraphics graph;
	graph.flags = WTW_GRAPH_FLAG_GENERATE_HBITMAP;
	graph.graphId = graphId;
	HBITMAP bitmap = reinterpret_cast<HBITMAP>(
		wtwPf->fnCall(WTW_GRAPH_GET_IMAGE, reinterpret_cast<WTW_PARAM>(&graph), NULL));
#endif
	MENUITEMINFO menuItemInfo;
	memset(&menuItemInfo, 0, sizeof(menuItemInfo));
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	menuItemInfo.hSubMenu = nullptr;
	menuItemInfo.hbmpChecked = bitmap;
	menuItemInfo.hbmpUnchecked = bitmap;
	menuItemInfo.hbmpItem = bitmap;

	SetMenuItemInfo(menu, itemPos, true, &menuItemInfo);
}
