/*
    Matrix Script Language
    author: Zeb
    e-mail: zebbey@gmail.com
    Copyright (c) 2004 Zeb.  All rights reserved.
*/
#ifndef __H_HEADER__
#define __H_HEADER__

#if defined(linux) || defined(__linux) || defined(__linux__)
#   define PLATFORM_LINUX
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#   define PLATFORM_WINDOWS
#elif defined(__APPLE__)
#	define PLATFORM_MACOS
#endif

/*
	LP64   unix64
	ILP64
	LLP64  win64
	ILP32  win32
	LP32
*/
#if defined(PLATFORM_WINDOWS)
#	if defined(WIN32)
#		define MODEL_ILP32
#	else
#		define MODEL_LLP64
#	endif
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>

#if defined(PLATFORM_WINDOWS)
#   include <conio.h>
#   include <windows.h>
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#   include <dlfcn.h>
#	include <sys/time.h>
#	include <unistd.h>
#else
#   error "unknown platform."
#endif

#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#   define MAX_PATH PATH_MAX
#endif

typedef long integer_t;
typedef float real_t;
#define REAL_FMT "%f"

#ifndef NULL
#   define NULL 0
#endif

#ifndef _countof
#   define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

#endif

