/*
* Copyright (C) 2011, WTW.im Sp. z o.o.
*/

#ifndef _LIBWTW_STANDALONE_
#ifdef LIBWTW_EXPORTS
#define LIBWTW_API __declspec(dllexport)
#else
#ifdef _EXP_DEF_SCK_
#define LIBWTW_API
#else
#define LIBWTW_API __declspec(dllimport)
#endif
#endif
#endif

#include <assert.h>

#ifndef ASSERT
#define ASSERT(f) assert(f)
#endif
