/*
*** Copyright (C) 2007-2015, K2T.eu
*/

#pragma once

#include "BaseObject.h"
#include <wtwI18N.h>

namespace wtw {

	class LIBWTW_API CTranslation : public CBaseObject
	{
	private:
		WTWFUNCTION _msg;
		void *m_pCbData;

	public:
		CTranslation();
		virtual ~CTranslation();

	public:
		const wchar_t * msg(const wchar_t * mod, DWORD id, const wchar_t *defaultText);
		HRESULT dumpWindowToXml(const wchar_t * fileFullPath, HWND hWnd);
		HRESULT translateHwnd(const wchar_t * mod, DWORD id, HWND hWnd);
		const wchar_t * getCurrentLngCode();
		int isLangAvailable(const wchar_t *lngCode);
		int messageBox(
			const wchar_t * mod, DWORD id, 
			const wchar_t * defaultText, const wchar_t * defaultTitle, int defaultRet,
			DWORD uType, HWND hParent, ...);
	};

};