// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#ifdef _WIN32

#ifdef _MSC_VER
//#define _CRT_SECURE_NO_DEPRECATE 1
//#define _ATL_SECURE_NO_DEPRECATE 1
//#define _CRT_NON_CONFORMING_SWPRINTFS 1
#endif

#define STRICT

#include "targetver.h"

#define _AFXDLL 1 // Required for <afxwin.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
//#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
//#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS         // remove support for MFC controls in dialogs

//#ifndef VC_EXTRALEAN
//#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
//#endif

// Windows Header Files:
#include <cstdio>			// must be before wtw includes
//#include <windows.h>
/*
	#include <afx.h>
	#include <afxwin.h>         // MFC core and standard components
	#include <afxext.h>         // MFC extensions
	#ifndef _AFX_NO_OLE_SUPPORT
	#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
	#endif
	#ifndef _AFX_NO_AFXCMN_SUPPORT
	#include <afxcmn.h>                     // MFC support for Windows Common Controls
	#endif // _AFX_NO_AFXCMN_SUPPORT

	#include <iostream>
	// Windows Header Files:
	#include <windows.h>
*/
#include <afxwin.h>

#pragma warning(disable: 4996) /*  'wcsncpy': This function or variable may be unsafe. Consider using wcsncpy_s instead */
#include <wtwapi/plInterface.h>
#pragma warning(default: 4996)

// Make it globally visible (but not exported)
extern WTWFUNCTIONS* wtw;
extern HINSTANCE hInstance;

#define TO_WTW_PARAM(x) reinterpret_cast<WTW_PARAM>(x)


#if 0	// WTL BEGIN
		//#define _WTL_FORWARD_DECLARE_CSTRING
		#define _WTL_NO_AUTOMATIC_NAMESPACE

	#ifdef _AFX
	#ifndef _WTL_NO_CSTRING
	#define _WTL_NO_CSTRING 1
	#endif // _WTL_NO_CSTRING
	#define _CSTRING_NS
	#endif // _AFX

		#include <atlbase.h>
		#include <atlapp.h>
	//#include <atlwin.h>
				
#endif	// WTL END


#endif
