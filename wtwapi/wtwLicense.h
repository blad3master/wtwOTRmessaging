/*
*** Copyright (C) 2007-2015, K2T.eu
*/

#pragma once

#include "wtwPlugin.h"

/*******************************************************************************
**
** Event:
**		WTW_ON_LICENSE_VERIFY
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - wtwLicenseInfo*
**		lP - NULL
**
*/

#define WTW_ON_LICENSE_VERIFY L"WTW/OnLicenseVerify"

struct wtwLicenseInfo
{
	__wtwStdStructDefs(wtwLicenseInfo);

	int			structSize;

	DWORD		type;
	DWORD		usersMax;
	DWORD		usersActive;
	QWORD		expires;

	wchar_t		licensedFor[128];
	wchar_t		licenseId[256];
	wchar_t		sysName[128];
};

/*******************************************************************************
**
** Function:
**		WTW_GET_LICENSE_TYPE
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - DWORD*
**		lP - NULL
**
*/

#define WTW_GET_LICENSE_TYPE	L"WTW/GetLicenseType"	// wP - DWORD*

#define WTW_LICENSE_TYPE_STANDARD	0x00000000
#define WTW_LICENSE_TYPE_CORPORATE	0x00000001

#define WTW_LICENSE_TYPE_MASK		0x0000FFFF

#define WTW_LICENSE_FLAG_INVALID	0x00010000
#define WTW_LICENSE_FLAG_INSTALLED	0x00020000

#define WTW_LICENSE_FLAG_MASK		0xFFFF0000

/*******************************************************************************
**
** Function:
**		WTW_GET_LICENSE_INFO
**
** Return value:
**		S_OK, E_*
**
** Parameters:
**		wP - wtwLicenseInfo*
**		lP - NULL
**
*/

#define WTW_GET_LICENSE_INFO	L"WTW/GetLicenseInfo"	// wP - wtwLicenseInfo*

