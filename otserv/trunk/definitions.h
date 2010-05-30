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

#define OTSERV_VERSION "0.6.3_SVN"
#define OTSERV_NAME "OTServ"
#define OTSERV_CLIENT_VERSION "8.57"
#define CURRENT_SCHEMA_VERSION 21

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
	#define __USE_SQLITE__
	#define SINGLE_SQL_DRIVER
#endif

enum passwordType_t{
	PASSWORD_TYPE_PLAIN = 0,
	PASSWORD_TYPE_MD5,
	PASSWORD_TYPE_SHA1
};

// Boost won't complain about non-working function
#define BOOST_ASIO_ENABLE_CANCELIO 1

#ifndef __FUNCTION__
	#define	__FUNCTION__ __func__
#endif

// Debug help only safety works with Visual C++
#ifdef __USE_MINIDUMP__
	#ifndef __EXCEPTION_TRACER__
		#define __EXCEPTION_TRACER__
	#endif
#endif

#define xmake_str(str) #str
#define make_str(str) xmake_str(str)

#if defined __GNUC__
	//GNU Compiler version example, GCC 3.0.2: 300002
	#ifdef __GNUC_PATCHLEVEL__
		#define __GNUC_VERSION__ (__GNUC__ * 10000 \
								+ __GNUC_MINOR__ * 100 \
								+ __GNUC_PATCHLEVEL__)
	#else
		#define __GNUC_VERSION__ (__GNUC__ * 10000 \
								+ __GNUC_MINOR__ * 100)
		#define __GNUC_PATCHLEVEL__ 0
	#endif
	#define COMPILER_STRING ("GCC " make_str(__GNUC__) "." \
								    make_str(__GNUC_MINOR__) "." \
									make_str(__GNUC_PATCHLEVEL__))

	//GNU Ports
	#ifdef __MINGW32__
		//MinGW port version example, MinGW 2.7: 2070
		#define __MINGW32_VERSION__ (__MINGW32_MAJOR_VERSION * 1000 \
								+ __MINGW32_MINOR_VERSION * 10)
		#define COMPILER_PORT_STRING ("MinGW32 " make_str(__MINGW32_MAJOR_VERSION) "." \
											     make_str(__MINGW32_MINOR_VERSION))
		#define __WINDOWS__
		#define XML_GCC_FREE
	#elif __MINGW64__
		#define COMPILER_PORT_STRING ("MinGW64 " make_str(__VERSION__))
		#define __WINDOWS__
	#elif __CYGWIN__
		#define COMPILER_PORT_STRING ("Cygwin " make_str(__VERSION__))
		#undef WIN32
		#undef _WIN32
		#undef __WIN32__
		#undef __WINDOWS__
		#define HAVE_ERRNO_AS_DEFINE
	#endif

	#if __GNUC_VERSION__ > 40300 // GCC 4.3.0
		#ifndef __GXX_EXPERIMENTAL_CXX0X__
			#include <tr1/unordered_map>
			#include <tr1/unordered_set>
			#define UNORDERED_MAP std::tr1::unordered_map
			#define UNORDERED_SET std::tr1::unordered_set
		#else
			#include <unordered_map>
			#include <unordered_set>
			#define UNORDERED_MAP std::unordered_map
			#define UNORDERED_SET std::unordered_set
		#endif		
	#elif __GNUC_VERSION__ > 20900 // GCC 2.9.0
		#include <ext/hash_map>
		#include <ext/hash_set>
		#define UNORDERED_MAP __gnu_cxx::hash_map
		#define UNORDERED_SET __gnu_cxx::hash_set
	#else
		#include <boost/version.hpp>
		#if BOOST_VERSION >= 103600 // Boost 1.36
			#include <boost/unordered_map.hpp>
			#include <boost/unordered_set.hpp>
			#define UNORDERED_MAP boost::unordered_map
			#define UNORDERED_SET boost::unordered_set
		#endif
	#endif

	#ifdef __GXX_EXPERIMENTAL_CXX0X__
		#include <cstdint>
	#else
		#include <stdint.h>
	#endif

	#include <cassert>
	#include <cstring>

	#define ATOI64 atoll

	#ifdef __USE_MINIDUMP__
		#undef __USE_MINIDUMP__
	#endif

#elif defined(_MSC_VER)
	//Visual C++ Compiler version
	#if _MSC_VER >= 1600 // VC++ 10.0
		#define VISUALC_VERSION 10
	#elif _MSC_VER >= 1500 // VC++ 9.0
		#define VISUALC_VERSION 9
	#elif _MSC_VER >= 1400 // VC++ 8.0
		#define VISUALC_VERSION 8
	#elif _MSC_VER >= 1310 // VC++ 7.1
		#define VISUALC_VERSION 7.1
	#elif _MSC_VER >= 1300 // VC++ 7.0
		#define VISUALC_VERSION 7
	#endif
	#define COMPILER_STRING ("Microsoft Visual C++ " make_str(VISUALC_VERSION))

	#define __WINDOWS__

	#ifdef NDEBUG
		#define _SECURE_SCL 0
		#define HAS_ITERATOR_DEBUGGING 0
	#endif

	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#include <limits>
	#include <cassert>
	#include <ctime>

	#include <hash_map>
	#include <hash_set>
	#define UNORDERED_MAP stdext::hash_map
	#define UNORDERED_SET stdext::hash_set

	#include <cstring>
	inline int strcasecmp(const char *s1, const char *s2)
	{
		return ::_stricmp(s1, s2);
	}

	inline int strncasecmp(const char *s1, const char *s2, size_t n)
	{
		return ::_strnicmp(s1, s2, n);
	}

	#if VISUALC_VERSION >= 10
		#include <stdint.h>
	#else
		typedef unsigned long long uint64_t;
		typedef signed long long int64_t;
		// Int is 4 bytes on all x86 and x86-64 platforms
		typedef unsigned int uint32_t;
		typedef signed int int32_t;
		typedef unsigned short uint16_t;
		typedef signed short int16_t;
		typedef unsigned char uint8_t;
		typedef signed char int8_t;
	#endif

	#define ATOI64 _atoi64

	#pragma warning(disable:4786) // msvc too long debug names in stl
	#pragma warning(disable:4250) // 'class1' : inherits 'class2::member' via dominance
	#pragma warning(disable:4244) //'argument' : conversion from 'type1' to 'type2', possible loss of data
	#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data
	#pragma warning(disable:4996) //"_ftime64" : this function or variable may be unsafe
#endif

#ifdef XML_GCC_FREE
	#define xmlFreeOTSERV(s)	free(s)
#else
	#define xmlFreeOTSERV(s)	xmlFree(s)
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

// OpenTibia configuration
#if !defined(__NO_SKULLSYSTEM__) && !defined(__SKULLSYSTEM__)
	#define __SKULLSYSTEM__
#endif

#endif
