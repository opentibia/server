//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// various definitions needed by most files
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_DEFINITIONS_H__
#define __OTSERV_DEFINITIONS_H__


#define OTSERV_VERSION "0.7.0"
#define OTSERV_NAME "OTServ"
#define CURRENT_SCHEMA_VERSION 25

#define CLIENT_VERSION_MIN 870
#define CLIENT_VERSION_MAX 870
#define CLIENT_VERSION_STRING "8.70"

#ifdef __USE_SQLITE__
	#define SINGLE_SQL_DRIVER
#endif

#ifdef __USE_MYSQL__
	#ifdef SINGLE_SQL_DRIVER
		#define MULTI_SQL_DRIVERS
	#else
		#define SINGLE_SQL_DRIVER
	#endif
#endif

#ifdef __USE_ODBC__
	#ifdef SINGLE_SQL_DRIVER
		#define MULTI_SQL_DRIVERS
	#else
		#define SINGLE_SQL_DRIVER
	#endif
#endif

#ifdef __USE_PGSQL__
	#ifdef SINGLE_SQL_DRIVER
		#define MULTI_SQL_DRIVERS
	#else
		#define SINGLE_SQL_DRIVER
	#endif
#endif

//Default sql driver
#if !defined(SINGLE_SQL_DRIVER) && !defined(MULTI_SQL_DRIVERS)
	#define SINGLE_SQL_DRIVER
#endif

enum passwordType_t{
	PASSWORD_TYPE_PLAIN = 0,
	PASSWORD_TYPE_MD5,
	PASSWORD_TYPE_SHA1
};

// Figure out build type
// __DEBUG__ / __RELEASE__ is the prefered form

#if defined _DEBUG
	#ifndef __DEBUG__
		#define __DEBUG__ 1
	#endif
#endif

#if defined DEBUG
	#ifndef __DEBUG__
		#define __DEBUG__ 1
	#endif
#elif defined __DEBUG__
	#ifndef DEBUG
		#define DEBUG 1
	#endif
#endif

#if defined RELEASE
	#ifndef __RELEASE__
		#define __RELEASE__ 1
	#endif
#elif defined __RELEASE__
	#ifndef RELEASE
		#define RELEASE 1
	#endif
#endif

#if defined __DEBUG__ && defined __RELEASE__
	#error "A build cannot be both debug and release, use either -D__DEBUG__ or -D__RELEASE__"
#endif

#if !defined __DEBUG__ && !defined __RELEASE__
	// Default to release
	#define __RELEASE__ 1
#endif

#ifdef __DEBUG__
	#define NDEBUG // Disabled assert
#endif

// Create assert macros

#include <assert.h>

#ifdef ASSERT
	#undef ASSERT
#endif
#ifdef ASSERT_MSG
	#undef ASSERT_MSG
#endif

#ifdef __DEBUG__
	// Debug break for when it's needed
	#ifdef _MSC_VER
		#define DEBUGBREAK() __debugbreak()
	#else
		#define DEBUGBREAK() exit(EXIT_FAILURE)
	#endif

	// Simple assert macro which also supports passing a message
	#define ASSERT_MSG(expr, msg) \
		do { \
			if (! (expr)) { \
				std::cerr << "Assertion `" << msg << "` failed" << std:: endl \
					<< __FILE__ << ": " << __LINE__ << " in function " << __FUNCTION__ << std::endl; \
				DEBUGBREAK(); \
			} \
		} \
		while (false)
#else
	#define ASSERT_MSG(expr) (void)0
#endif

#define ASSERT(expr) ASSERT_MSG(expr, #expr)

#ifndef __FUNCTION__
	#define	__FUNCTION__ __func__
#endif

#define xmake_str(str) #str
#define make_str(str) xmake_str(str)

/*
	Compiler setup
*/
#if defined __GNUC__
	#ifdef __llvm__
		#include "compiler/llvm.h"
	#else
		#include "compiler/gcc.h"
		#ifdef __MINGW32__
			#include "compiler/mingw32.h"
		#endif
	#endif
#elif defined(_MSC_VER)
	#include "compiler/msvc.h"
#endif

/*
	If the compiler supports the upcoming standard,
	call some of the useful headers.
*/
#ifdef __OTSERV_CXX0X__
	#include <cstdint>
	#include <unordered_map>
	#include <unordered_set>
#else
	#include "compiler/workarounds.h"
#endif

#ifdef _WIN32_WINNT
	#undef _WIN32_WINNT
#endif
//Windows 2000	0x0500
//Windows Xp	0x0501
//Windows 2003	0x0502
//Windows Vista	0x0600
//Windows Seven 0x0601
#define _WIN32_WINNT 0x0501

#ifdef __DEBUG_EXCEPTION_REPORT__
	#define DEBUG_REPORT int *a = NULL; *a = 1;
#else
	#ifdef __EXCEPTION_TRACER__
		#include "exception.h"
		#define DEBUG_REPORT ExceptionHandler::dumpStack();
	#else
		#define DEBUG_REPORT
	#endif
#endif

//#define __MIN_PVP_LEVEL_APPLIES_TO_SUMMONS__ //experimental

// OpenTibia configuration
#if !defined(__NO_SKULLSYSTEM__) && !defined(__SKULLSYSTEM__)
	#define __SKULLSYSTEM__
#endif

#endif
