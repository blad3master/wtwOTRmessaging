#pragma once

#include "wtwPlugin.h"

/*******************************************************************************
**
** Event:
**		WTW_ON_SHARED_CONTACT_SYNC_COMPLETE
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - NULL
**		lP - NULL
**
*/

#define WTW_ON_SHARED_CONTACT_SYNC_COMPLETE L"WTW/SC/SyncComplete"

/*******************************************************************************
**
** Function:
**		WTW_SHARED_CONTACT_LIST_QUERY
**
** Return value:
**		S_OK, S_FALSE, E_*
**
** Parameters:
**		wP - wtwSharedContactQuery*
**		lP - wtwSharedContact**
**
*/

#define WTW_SHARED_CONTACT_LIST_QUERY L"WTW/SC/ListQuery"

struct wtwSharedContact
{
	__wtwStdStructDefs(wtwSharedContact);

	int structSize;

	const wchar_t * id;
	const wchar_t * reservedc;
	const wchar_t * name;
	const wchar_t * group;

	int mode;
	int type;

	void *reserved1;
	void *reserved2;

	wtwSharedContact *pNext;
};

#define WTW_SC_MODE_NO_OVERRIDE		0
#define WTW_SC_MODE_FORCE_OVERRIDE	1
#define WTW_SC_MODE_DELETE			2

#define WTW_SC_TYPE_GHOST	0
#define WTW_SC_TYPE_NORMAL	1

struct wtwSharedContactQuery
{
	__wtwStdStructDefs(wtwSharedContactQuery);

	int structSize;

	const wchar_t * accountId; // optional
	const wchar_t * netClass;

	DWORD	flags;

	void * reserved1;
	void * reserved2;
};

/*******************************************************************************
**
** Function:
**		WTW_SHARED_CONTACT_LIST_FREE
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - wtwSharedContact*
**		lP - NULL
**
*/

#define WTW_SHARED_CONTACT_LIST_FREE L"WTW/SC/ListFree"
