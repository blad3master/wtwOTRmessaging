#pragma once

#include "wtwPlugin.h"

#define WTW_WEB_AUTH_EVENT_VERIFY_URL	0x0001
#define WTW_WEB_AUTH_EVENT_DESTROY		0x0002

struct wtwWebAuthUrl
{
	__wtwStdStructDefs(wtwWebAuthUrl);

	int			structSize;

	const wchar_t * pUrl;
};

struct wtwWebAuthWindow
{
	__wtwStdStructDefs(wtwWebAuthWindow);

	int			structSize;

	HWND			hParent;
	WTWFUNCTION		callback;
	void *			cbData;

	const wchar_t * initialUrl;
	const wchar_t * sandboxUrl;

	const wchar_t * windowTitle;
	const wchar_t * windowHint;

	DWORD			flags;
};

#define WTW_WEB_AUTH_SHOW_WINDOW	L"WTW/WebAuthShowWnd"