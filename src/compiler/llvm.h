//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// LLVM compiler configuration
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

#ifndef __OTSERV_LLVM_H__
#define __OTSERV_LLVM_H__

#define __OSX__

#include <tr1/unordered_map>
#include <tr1/unordered_set>
#define __OTSERV_UNORDERED_MAP_WORKAROUND__ tr1::unordered_map
#define __OTSERV_UNORDERED_SET_WORKAROUND__ tr1::unordered_set
#define __OTSERV_FUNCTIONAL_HASH_WORKAROUND__ tr1::hash

/*
  String to 64 bit integer macro
*/
#define ATOI64 atoll

#endif
