//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The EventScheduler manages events and calls callbacks when an
// event happened.
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
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.11  2003/11/01 15:52:43  tliffrag
// Improved eventscheduler
//
// Revision 1.10  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.9  2002/08/01 14:11:28  shivoc
// added initial support for 6.9x clients
//
// Revision 1.8  2002/05/28 13:55:56  shivoc
// some minor changes
//
// Revision 1.7  2002/04/24 18:25:39  shivoc
// some changes for win compatibility
//
// Revision 1.6  2002/04/08 15:57:03  shivoc
// made some changes to be more ansi compliant
//
// Revision 1.5  2002/04/05 18:56:11  acrimon
// Adding a file class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_EVENTSCHEDULER_H
#define __OTSERV_EVENTSCHEDULER_H
#include <map>
#include <sys/timeb.h>
#if __GNUC__ < 3
#include <hash_map>
#else
#include <ext/hash_map>
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
using namespace __gnu_cxx;
#endif
#if (__GNUC__ == 3 && __GNUC_MINOR__ < 1)
using std::hash_map;
#endif
#endif

#include "tmap.h"
#include "definitions.h"
#include "network.h"

struct eqfd {
    bool operator() (Socket s1, Socket s2) const {
        return s1 == s2;
    }
};

struct cmpdouble {
  bool operator()(double s1, double s2) const {
    return s1 < s2;
  }
};

enum tickTypes{
	TICK_MAP,
	TICK_CREATURE
};

struct stick {
	int type;
	int cid;
};

typedef hash_map<Socket, unary_functor<Socket,void> *, hash<Socket>, eqfd> fdcbhash;

// EventListener ?
class EventScheduler {
	fdcbhash fdcb;
	fd_set active_fd_set, read_fd_set;

	std::multimap<double, stick*, cmpdouble> tickList;

	public:
    EventScheduler();
    void newsocket(Socket sock, unary_functor<Socket,void> *);
	void deletesocket(Socket sock);
    void loop();
	int addMapTick(int ms);
	int addCreatureTick(long c, int ms);
	double getNow(){
		timeb nowTime;
		ftime(&nowTime);
		return (double)nowTime.time+(double)nowTime.time/1000;
	}
};

#endif
