#pragma once

#include <wtwPlugin.h>

#ifdef _LIBWTW_STANDALONE_
#define LIBWTW_API
#define ASSERT(x)
#else
#include "libWTW.h"
#endif