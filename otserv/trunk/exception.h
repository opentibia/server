//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Exception Handler class
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

#if defined __EXCEPTION_TRACER__

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include "definitions.h"

class ExceptionHandler
{
public:
	ExceptionHandler();
	~ExceptionHandler();
	bool InstallHandler();
	bool RemoveHandler();
	static void dumpStack();
private:
#if defined __WINDOWS__

	#if defined _MSC_VER || defined __USE_MINIDUMP__

		static long __stdcall MiniDumpExceptionHandler(struct _EXCEPTION_POINTERS *pExceptionInfo);
		static int ref_counter;

	#elif __GNUC__

		struct SEHChain{
				SEHChain *prev;
				void *SEHfunction;
			};
			SEHChain chain;
			bool LoadMap();
			static bool isMapLoaded;
		#endif

	#endif

	bool isInstalled;
};

#endif

#endif
