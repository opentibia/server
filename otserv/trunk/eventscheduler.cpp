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

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include "eventscheduler.h"

EventScheduler es;

EventScheduler::EventScheduler() {
  FD_ZERO(&active_fd_set);
  FD_ZERO(&read_fd_set);
}

//////////////////////////////////////////////////
// If an open socket should be listened to, call this.
void EventScheduler::newsocket(signed int sock, unary_functor<int,void> *cb) {
  FD_SET(sock, &active_fd_set);
  fdcb[sock] = cb;
  printf("new socket to listen: %d   %d\n", sock, cb);
}

//////////////////////////////////////////////////
// The socket should not be listened to anymore.
void EventScheduler::deletesocket(signed int sock) {
  FD_CLR(sock, &active_fd_set);
  fdcb.erase(sock);
  // fehlen da nicht noch andere callbacks?
}

//////////////////////////////////////////////////
// main loop, listening to events and calling callbacks.
// It does not return.
// Todo: optionally returning when no socket is listened to anymore.
void EventScheduler::loop() {
  struct timeval tv = {1, 0};
  for (;;) {
    if (tv.tv_sec == 0 && tv.tv_usec == 0) {
      cout << "time event" << endl;
      tv.tv_sec = 1;
    }
    read_fd_set = active_fd_set;
    int sel = select(FD_SETSIZE, &read_fd_set, NULL, NULL, &tv);
    if (sel < 0) {
      perror("select");
      exit(-1);
    }
    cout << "loop ";
    for (int i = 0; sel && i < FD_SETSIZE; i++) {
      if (FD_ISSET(i, &read_fd_set) > 0) { // if there was input:
	sel--;
	cout << "socket event " << i << endl;
	(*fdcb[i])(i);  // call the callback
      }
    }
  }
}
