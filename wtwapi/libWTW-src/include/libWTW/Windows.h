/*
*** Copyright (C) 2007-2015, K2T.eu
*/

#pragma once

#include <wtwPlugin.h>
#include <wtwWindows.h>
#include "Core.h"
#include "String.h"
#include "BaseObject.h"
#include "SettingsHelper.h"

namespace wtw 
{
	class LIBWTW_API CWindowBase : public CBaseObject
	{
	public:
		CWindowBase(const CWindowBase&) = delete;

	public:
		CWindowBase();
		virtual ~CWindowBase();

	protected:
		WTWFUNCTION _fnCreate;
		void *_fnCreateData;
		WTWFUNCTION _fnDestroy;
		void *_fnDestroyData;
		WTWFUNCTION _fnShow;
		void *_fnShowData;
		WTWFUNCTION _fnMove;
		void *_fnMoveData;
		WTWFUNCTION _fnAddControl;
		void *_fnAddControlData;
		WTWFUNCTION _fnSet;
		void *_fnSetData;
		WTWFUNCTION _fnGet;
		void *_fnGetData;
		WTWFUNCTION _fnOptsLoad;
		void *_fnOptsLoadData;
		WTWFUNCTION _fnOptsSave;
		void *_fnOptsSaveData;

	protected:
		void initialize();
		void* m_pWnd;
		static WTW_PTR onWindowEvent(WTW_PARAM wP, WTW_PARAM lP, void *od);

	protected:
		virtual void onInitWindow();
		virtual void onDestroy();
		virtual void onListItemSelect(const wchar_t * id, const wtwWindowListItem & item);
		virtual void onButtonClicked(const wchar_t * id);
		virtual void onCheckboxClicked(const wchar_t * id, const int checked);

	public:
		HRESULT destroy();
		HRESULT create(const wchar_t *id, const wchar_t *title, int x, int y, int cx, int cy, int type, HWND hParent);
		HRESULT show(int cmd);
		HRESULT move(int x, int y, int cx, int cy);
		HRESULT addControl(
			const wchar_t *id, const wchar_t *text, int x, int y, int cx, int cy, int type,
			int resizeMode = WTW_WINDOW_CRM_NONE, const wchar_t *parentId = nullptr, wtwWindowControlSettingBinder *pSettings = nullptr,
			int mL = 0, int mR = 0, DWORD flags = 0);
		HRESULT getCheck(const wchar_t * id, int & ret);
		HRESULT setCheck(const wchar_t * id, int ret);
		HRESULT addListItem(
			const wchar_t *id, const wchar_t * text, int *pIndex = nullptr);
		HRESULT setCurSel(
			const wchar_t *id, int index);
		HRESULT clear(
			const wchar_t *id);
	};

	class LIBWTW_API CWindowSettings : public CWindowBase
	{
	public:
		CWindowSettings(const CWindowSettings&) = delete;

	public:
		CWindowSettings();
		virtual ~CWindowSettings();

	protected:
		void* m_pSettings;
		int m_y;

	protected:
		virtual void onInitWindow();
		virtual void onDestroy();

	public:
		HRESULT create(const wchar_t *id, int cy, HWND hParent, CSettings *pSettings);
		HRESULT addCheckbox(
			const wchar_t *id, const wchar_t *text, int indent, const wchar_t *parentId, int defaultValue);
		HRESULT addSeparator(
			const wchar_t *id, const wchar_t *text);
		HRESULT addEdit(
			const wchar_t *id, const wchar_t *text, int indent, const wchar_t *parentId, const wchar_t * defaultValue);
		HRESULT addEditPassword(
			const wchar_t *id, const wchar_t *text, int indent, const wchar_t *parentId, const wchar_t * defaultValue);
		HRESULT addLabel(
			const wchar_t *id, const wchar_t *text, int indent, const wchar_t *parentId);
		HRESULT addDropList(
			const wchar_t *id, int indent, const wchar_t *parentId, int defaultIndex);

		HRESULT optionsLoad();
		HRESULT optionsSave();
	};

}