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

#include <hash_map>

#include "definitions.h"
#include "network.h"

struct eqfd {
  bool operator() (Socket s1, Socket s2) const {
    return s1 == s2;
  }
};

typedef std::hash_map<Socket, unary_functor<Socket,void> *, std::hash<Socket>, eqfd> fdcbhash;

// EventListener ?
class EventScheduler {
  fdcbhash fdcb;
  fd_set active_fd_set, read_fd_set;
public:
  EventScheduler();
  void newsocket(Socket sock, unary_functor<Socket,void> *);
  void deletesocket(Socket sock);
  void loop();
};
