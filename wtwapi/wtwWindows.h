/*
** Copyright (C) 2007-2015, K2T.eu
*/

#pragma once

#include "wtwPlugin.h"

/*******************************************************************************
**
** Function:
**		WTW_WINDOW_CREATE
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - wtwWindow*
**		lP - void ** - on success, window pointer
**
*/

#define WTW_WINDOW_CREATE L"WTW/Window/Create"

struct wtwWindow
{
	__wtwStdStructDefs(wtwWindow);

	int				structSize;

	const wchar_t	*title;
	const wchar_t	*id;
	
	WTWFUNCTION		eventCallback; // can be null, wP = wtwWindowEvent*
	void			*eventCallbackData;
	
	int				type;

	int				x;
	int				y;
	int				cx;
	int				cy;

	DWORD			flags;

	HWND			hParent;

	void			*reserved1;
	void			*reserved2;
};

#define WTW_WINDOW_TYPE_CHILD_SCROLLABLE	0x01

#define WTW_WINDOW_FLAG_NO_DPI_SCALING		0x00000001

struct wtwWindowEvent
{
	__wtwStdStructDefs(wtwWindowEvent);

	int				structSize;

	int				event;
	const wchar_t	*windowId;
	const wchar_t	*controlId;
	const void		*ptr;

};

struct wtwWindowListItem
{
	__wtwStdStructDefs(wtwWindowListItem);

	int				structSize;

	int				index;
	int				itemTextMax;	// unused for set
	const wchar_t	*itemText;
	DWORD_PTR		itemData;

	DWORD			flags;
};

#define WTW_WINDOW_EVENT_INITIALIZE		0x0001 // window initialized
#define WTW_WINDOW_EVENT_DESTROY		0x0002 // window is being destroyed
#define WTW_WINDOW_EVENT_ITEM_SELECT	0x0003 // combo/droplist/list item has been selected, 
#define WTW_WINDOW_EVENT_BUTTON_CLICK	0x0004 // 
#define WTW_WINDOW_EVENT_CHECKBOX_CLICK	0x0005 // 

/*******************************************************************************
**
** Function:
**		WTW_WINDOW_DESTROY
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - void* - window pointer from _CREATE
**		lP - NULL
**
*/

#define WTW_WINDOW_DESTROY L"WTW/Window/Destroy"

/*******************************************************************************
**
** Function:
**		WTW_WINDOW_MOVE
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - wtwWindowPos*
**		lP - NULL
**
*/

#define WTW_WINDOW_MOVE L"WTW/Window/Move"

struct wtwWindowPos
{
	__wtwStdStructDefs(wtwWindowPos);

	int				structSize;

	void			*ptr;

	int				x;
	int				y;
	int				cx;
	int				cy;

	DWORD			flags;

	void			*reserved1;
	void			*reserved2;
};

/*******************************************************************************
**
** Function:
**		WTW_WINDOW_SHOW
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - wtwWindowShow*
**		lP - NULL
**
*/

#define WTW_WINDOW_SHOW L"WTW/Window/Show"

struct wtwWindowShow
{
	__wtwStdStructDefs(wtwWindowShow);

	int				structSize;

	void			*ptr;

	int				cmd;
	DWORD			flags;

	void			*reserved1;
	void			*reserved2;
};

#define WTW_WINDOW_SHOW_CMD_HIDE		0x00
#define WTW_WINDOW_SHOW_CMD_SHOW		0x01
#define WTW_WINDOW_SHOW_CMD_MAXIMIZE	0x03
#define WTW_WINDOW_SHOW_CMD_MINIMIZE	0x04
#define WTW_WINDOW_SHOW_CMD_RESTORE		0x05

/*******************************************************************************
**
** Function:
**		WTW_WINDOW_ADD_CONTROL
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - wtwWindowControl*
**		lP - NULL
**
*/

#define WTW_WINDOW_ADD_CONTROL L"WTW/Window/AddControl"

struct wtwWindowControlSettingBinder
{
	__wtwStdStructDefs(wtwWindowControlSettingBinder);

	int				structSize;

	void			*ptr;
	int				type;
	
	union
	{
		long long	defaultInt;
		const wchar_t *defaultStr;
	};
};

struct wtwWindowControl
{
	__wtwStdStructDefs(wtwWindowControl);

	int structSize;

	void			*ptr; // windowpointer

	const wchar_t	*id;
	const wchar_t	*text;
	const wchar_t	*parentId; // optional

	int				controlType; // WTW_WINDOW_CT_*

	int				x;
	int				y;
	int				cx;
	int				cy;

	int				resizeMode; // WTW_WINDOW_CRM_*
	int				resizeMarginR;
	int				resizeMarginL;
	int				resizeMarginT;
	int				resizeMarginB;

	DWORD			flags; // WTW_WINDOW_CF_*

	wtwWindowControlSettingBinder *pBinder; // optional

	void			*reserved1;
	void			*reserved2;
};

#define WTW_WINDOW_CF_NO_DPI_SCALING			0x00000001
#define WTW_WINDOW_CF_RIGHT						0x00100000
#define WTW_WINDOW_CF_LEFT						0x00200000
#define WTW_WINDOW_CF_CENTER					0x00400000
#define WTW_WINDOW_CF_READ_ONLY					0x00800000
#define WTW_WINDOW_CF_MULTILINE					0x01000000
#define WTW_WINDOW_CF_PASSWORD					0x02000000
#define WTW_WINDOW_CF_NUMBER					0x04000000

#define WTW_WINDOW_CF_ENABLE_IF_PARENT_CHECKED	0x00010000

#define WTW_WINDOW_CT_SEPARATOR		0x00
#define WTW_WINDOW_CT_CHECKBOX		0x01
#define WTW_WINDOW_CT_LABEL			0x02
#define WTW_WINDOW_CT_EDIT			0x03
#define WTW_WINDOW_CT_COMBOBOX		0x04
#define WTW_WINDOW_CT_DROPLIST		0x05
#define WTW_WINDOW_CT_BUTTON		0x06

#define WTW_WINDOW_CRM_NONE			0x00
#define WTW_WINDOW_CRM_FULL_WIDTH	0x01
#define WTW_WINDOW_CRM_WIDTH		0x02
#define WTW_WINDOW_CRM_CENTER_H		0x03
#define WTW_WINDOW_CRM_RIGHT		0x04

/*******************************************************************************
**
** Function:
**		WTW_WINDOW_SET
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - wtwWindowSet*
**		lP - NULL
**
*/

#define WTW_WINDOW_SET L"WTW/Window/Set"

struct wtwWindowSet
{
	__wtwStdStructDefs(wtwWindowSet);

	int				structSize;

	void			*ptr;
	int				what;
	const wchar_t	*controlId;

	void			*pData;
};

#define WTW_WINDOW_PARAM_ENABLED		0x0001	// pData = int*
#define WTW_WINDOW_PARAM_CHECKED		0x0002	// pData = int*
#define WTW_WINDOW_PARAM_VISIBLE		0x0003	// pData = int*
#define WTW_WINDOW_PARAM_TEXT			0x0004	// pData = const wchar_t*
#define WTW_WINDOW_PARAM_TEXT_INT		0x0005	// pData = long long*
#define WTW_WINDOW_PARAM_LIST_ITEM		0x0006	// pData = wtwWindowListItem*
#define WTW_WINDOW_PARAM_SET_CUR_SEL	0x0007	// pData = int*
#define WTW_WINDOW_PARAM_DELETE_ITEM	0x0008	// pData = int*
#define WTW_WINDOW_PARAM_CLEAR			0x0009	// pData = nullptr

/*******************************************************************************
**
** Function:
**		WTW_WINDOW_GET
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - wtwWindowGet*
**		lP - NULL
**
*/

#define WTW_WINDOW_GET L"WTW/Window/Get"

struct wtwWindowGet
{
	__wtwStdStructDefs(wtwWindowGet);

	int				structSize;

	void			*ptr;
	int				what;
	const wchar_t	*controlId;

	void			*pData;
	int				dataSize;
};


/*******************************************************************************
**
** Function:
**		WTW_WINDOW_OPTIONS_SAVE
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - void* - window ptr
**		lP - NULL
**
** Remarks:
**		If pointer to settings interface was specified and control has valid id set
**		function stores control state into settings interface using it's id as a key
**
*/

#define WTW_WINDOW_OPTIONS_SAVE L"WTW/Window/OptSave"

/*******************************************************************************
**
** Function:
**		WTW_WINDOW_OPTIONS_LOAD
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - void* - window ptr
**		lP - NULL
**
*/

#define WTW_WINDOW_OPTIONS_LOAD L"WTW/Window/OptLoad"

