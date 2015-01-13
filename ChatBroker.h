#pragma once

#include "stdafx.h"

extern "C" {
#include "OTRL/proto.h"
}

#include <map>
#include <vector>
#include <string>


class ChatBroker
{
public:
	ChatBroker(void);
	~ChatBroker(void);

	// if contact is not nullptr, assume is the one for which button should be displayed
	// (no check if it is currently active)
	// if nullptr is provided - try to guess which one is active and update button for that one
	static void update_ui(const wtwContactDef * const activeContact);

private:

	static const wchar_t * const GraphId_notPrivate;
	static const wchar_t * const GraphId_private;
	static const wchar_t * const GraphId_unverified;
	static const wchar_t * const GraphId_help;
	static const wchar_t * const GraphId_identity;
	static const wchar_t * const GraphId_refresh;

	enum POPUP_ENABLE {
		START			= 0x01,
		REFRESH			= 0x02,
		FINISH			= 0x04,
		AUTHENTICATE	= 0x08,
		REAUTHENTICATE	= 0x10
	};

	HANDLE onChatWndCreateHook;
	HANDLE onChatWndDestroyHook;
	HANDLE onChatWndShowHook;
	HANDLE onChatWndMtcChangeHook;

	// makeKey(id,netClass,netId)  =>  chat window handle
	static std::map<std::wstring, void*> chatWndList;

	static std::wstring makeKey(const wchar_t *id, const wchar_t *netClass, int netId);
	static std::wstring makeKey(const wtwContactDef *contact);
	static bool wtwContactFromKey(const std::wstring &key, wchar_t *id, wchar_t *netClass, int *netId, size_t strLenBytes);

	static WTW_PTR onChatWndCreate(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr);
	
	static WTW_PTR onChatWndDestroy(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr);

	static WTW_PTR onChatWndShow(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr);

	static WTW_PTR onChatWndMetacontactChange(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr);

	static WTW_PTR on_WtwOtrmessaging_btn_clicked(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr);

	static int initializePrivateConversation(wtwContactDef *contact);

	static int finishPrivateConversation(wtwContactDef *contact);

	static void authenticatePeer(wtwContactDef *contact);

	static void loadGraphics();

	static void setMenuItemIcon(HMENU menu, UINT itemPos, const wchar_t *graphId);
};
