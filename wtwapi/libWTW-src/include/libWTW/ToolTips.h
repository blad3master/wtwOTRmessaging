/*
*** Copyright (C) 2007-2015, K2T.eu
*/

#pragma once

#include <wtwToolTips.h>
#include "Core.h"
#include "BaseObject.h"

namespace wtw {

	class LIBWTW_API CToolTips : public CBaseObject
	{
	private:
		HWND m_hWnd;

	public:
		CToolTips();
		virtual ~CToolTips();

	public:
		int create(HWND hParentWindow);
		int destroy();
		int addTool(DWORD ctrlId, const wchar_t *szSimpleHtmlText);
		int addTool(HWND hCtrl, const wchar_t *szSimpleHtmlText);
	};
};