//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// GCC compiler configuration
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

#ifndef __OTSERV_GCC_H__
#define __OTSERV_GCC_H__

/*
	Versioning macro for GNU C++ compiler.
	Example: GCC 3.0.2 = 300002
*/
#ifdef __GNUC_PATCHLEVEL__
	#define __GNUC_VERSION__ (__GNUC__ * 10000 \
							+ __GNUC_MINOR__ * 100 \
							+ __GNUC_PATCHLEVEL__)
#else
	#define __GNUC_VERSION__ (__GNUC__ * 10000 \
							+ __GNUC_MINOR__ * 100)
	#define __GNUC_PATCHLEVEL__ 0
#endif

/*
	C++0x workarounds for unordered map and unordered set.
	Since GCC embeds two or more STL implementations for TR1
	we need to check the one which better fits our needs.
	If the compiler supports C++0x, disable the workarounds
	and call the standard headers.
*/
#ifdef __GXX_EXPERIMENTAL_CXX0X__
	#define __OTSERV_CXX0X__
#else
	#if __GNUC_VERSION__ > 40300 // GCC 4.3.0
		#include <tr1/unordered_map>
		#include <tr1/unordered_set>
		#define __OTSERV_UNORDERED_MAP_WORKAROUND__ tr1::unordered_map
        #define __OTSERV_UNORDERED_SET_WORKAROUND__ tr1::unordered_set
        #define __OTSERV_FUNCTIONAL_HASH_WORKAROUND__ tr1::hash
	#elif __GNUC_VERSION__ > 30200 // GCC 3.2.0
		#include <ext/hash_map>
		#include <ext/hash_set>
		#define __OTSERV_UNORDERED_MAP_WORKAROUND__ __gnu_cxx::hash_map
		#define __OTSERV_UNORDERED_SET_WORKAROUND__ __gnu_cxx::hash_set
		#define __OTSERV_FUNCTIONAL_HASH_WORKAROUND__ __gnu_cxx::hash
	#endif
	#include <stdint.h>
#endif

/*
	Minidump for Windows platforms.
	Should be disabled at UNIX platforms.
*/
#ifdef __USE_MINIDUMP__
	#undef __USE_MINIDUMP__
#endif

/*
	String to 64 bit integer macro
*/
#define ATOI64 atoll

#define COMPILER_STRING ("GCC " make_str(__GNUC__) "." \
								make_str(__GNUC_MINOR__) "." \
								make_str(__GNUC_PATCHLEVEL__))

#endif // __OTSERV_GCC_H__
