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


#ifndef __DEFINITIONS_H__
#define __DEFINITIONS_H__

typedef unsigned long long uint64_t;

#ifdef XML_GCC_FREE
#define xmlFreeOTSERV(s)	free(s)
#else
#define xmlFreeOTSERV(s)	xmlFree(s)
#endif

#ifdef __DEBUG_EXCEPTION_REPORT__
#define DEBUG_REPORT int *a = NULL; *a = 1;
#else
#define DEBUG_REPORT
#endif

#if defined __WINDOWS__ || defined WIN32

#define OTSYS_THREAD_RETURN  void
#define EWOULDBLOCK WSAEWOULDBLOCK

#ifndef __GNUC__
#include <cstring>
inline int strcasecmp(const char *s1, const char *s2)
{
	return ::_stricmp(s1, s2);
}

typedef unsigned long uint32_t;
typedef signed long int32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif

#pragma warning(disable:4786) // msvc too long debug names in stl
#pragma warning(disable:4250) // 'class1' : inherits 'class2::member' via dominance

#ifndef NOMINMAX
#define NOMINMAX
#endif

#else

#define OTSYS_THREAD_RETURN void*

#include <stdint.h>
#include <string.h>

typedef int64_t __int64;

#endif


#endif // __DEFINITIONS_H__
