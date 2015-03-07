//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Visual C++ compiler configuration
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

#ifndef __OTSERV_MSVC_H__
#define __OTSERV_MSVC_H__

/*
  Visual Studio simple version definition.
  Releases before Visual Studio 2005 (8.0) are not supported
  but may compile otserv.
*/
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

/*
  If the compiler supports C++0x, disable the workarounds
  and call the standard headers.
*/
#if VISUALC_VERSION >= 10
  #define __OTSERV_CXX0X__
#else
  typedef unsigned long long uint64_t;
  typedef signed long long int64_t;
  typedef unsigned int uint32_t;
  typedef signed int int32_t;
  typedef unsigned short uint16_t;
  typedef signed short int16_t;
  typedef unsigned char uint8_t;
  typedef signed char int8_t;

  /*
    Unordered set and unordered map workarounds
  */
  #include <hash_map>
  #include <hash_set>
  #define __OTSERV_UNORDERED_MAP_WORKAROUND__ stdext::hash_map
  #define __OTSERV_UNORDERED_SET_WORKAROUND__ stdext::hash_set
  #define __OTSERV_FUNCTIONAL_HASH_WORKAROUND__ stdext::hash_compare
#endif

/*
  Debug help only safety works with Visual C++
*/
#ifdef __USE_MINIDUMP__
  #ifndef __EXCEPTION_TRACER__
    #define __EXCEPTION_TRACER__
  #endif
#endif

#pragma warning(disable:4250) // 'class1' : inherits 'class2::member' via dominance
#pragma warning(disable:4786) // msvc too long debug names in stl
#pragma warning(disable:4244) // 'argument' : conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4267) // 'var' : conversion from 'size_t' to 'type', possible loss of data

/*
  String to 64 bit integer macro
*/
#define ATOI64 _atoi64

#define COMPILER_STRING ("Microsoft Visual C++ " make_str(VISUALC_VERSION))

#endif // __OTSERV_MSVC_H__
