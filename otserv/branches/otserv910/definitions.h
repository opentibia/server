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
#define OTSERV_CLIENT_VERSION "8.70"

#ifndef __OLD_GUILD_SYSTEM__
#define __OLD_GUILD_SYSTEM__
#endif

#ifdef __OLD_GUILD_SYSTEM__
#define CURRENT_SCHEMA_VERSION 25
#else
#define CURRENT_SCHEMA_VERSION 22
#endif

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

#define xmake_str(str) #str
#define make_str(str) xmake_str(str)

/*
	Compiler setup
*/
#if defined __GNUC__
	#include "compiler/gcc.h"
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


// OpenTibia configuration
#if !defined(__NO_SKULLSYSTEM__) && !defined(__SKULLSYSTEM__)
	#define __SKULLSYSTEM__
#endif

// Boost exception handling must be enabled
#ifdef BOOST_NO_EXCEPTIONS
	#error "Boost exception handling must be enabled."
#endif

#endif
