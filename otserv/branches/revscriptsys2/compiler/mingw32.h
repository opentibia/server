//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// MingW32 compiler configuration
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

#ifndef __OTSERV_MINGW32_H__
#define __OTSERV_MINGW32_H__

/*
	Defines the operational system.
*/
#define __WINDOWS__

/*
	Versioning macro for MinGW port.
	Example: MinGW 2.7 = 2070
*/
#define __MINGW32_VERSION__ (__MINGW32_MAJOR_VERSION * 1000 \
							+ __MINGW32_MINOR_VERSION * 10)

#define COMPILER_PORT_STRING ("MinGW32 " make_str(__MINGW32_MAJOR_VERSION) "." \
										 make_str(__MINGW32_MINOR_VERSION))

#ifndef XML_GCC_FREE										 
	#define XML_GCC_FREE
#endif

#endif // __OTSERV_MINGW32_H__
