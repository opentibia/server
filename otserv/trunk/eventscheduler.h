/* OpenTibia - an opensource roleplaying game
 *
 * The EventScheduler manages events and calls callbacks when an
 * event happened.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <hash_map>
#include "definitions.h"

struct eqfd {
  bool operator() (Socket s1, Socket s2) const {
    return s1 == s2;
  }
};

typedef hash_map<Socket, unary_functor<Socket,void> *, hash<Socket>, eqfd> fdcbhash;

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
