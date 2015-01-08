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

	static void update_ui();

private:
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

	// contact->id  =>  chat window handle
	static std::map<std::wstring, void*> chatWndList;

	static std::wstring makeKey(const wchar_t *id, const wchar_t *netClass, int netId);

	static WTW_PTR onChatWndCreate(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr);
	
	static WTW_PTR onChatWndDestroy(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr);

	static WTW_PTR onChatWndShow(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr);

	static WTW_PTR moj_callback(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr);

	static int initializePrivateConversation(wtwContactDef *contact);

	static int finishPrivateConversation(wtwContactDef *contact);

	static void authenticatePeer(wtwContactDef *contact);
};
