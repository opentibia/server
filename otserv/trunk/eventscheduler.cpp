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
// Revision 1.5  2002/04/08 15:57:03  shivoc
// made some changes to be more ansi compliant
//
// Revision 1.4  2002/04/08 13:53:59  acrimon
// Added some very basic map support
//
// Revision 1.3  2002/04/05 19:44:07  shivoc
// added protokoll 6.5 inital support
//
// Revision 1.2  2002/04/05 18:19:28  acrimon
// test commit
//
//////////////////////////////////////////////////////////////////////

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

#include "eventscheduler.h"

extern EventScheduler es;

EventScheduler::EventScheduler() {
  FD_ZERO(&active_fd_set);
  FD_ZERO(&read_fd_set);
}

//////////////////////////////////////////////////
// If an open socket should be listened to, call this.
void EventScheduler::newsocket(Socket _sock, unary_functor<Socket,void> *_cb) {
  FD_SET(_sock, &active_fd_set);
  fdcb[_sock] = _cb;
  printf("new socket to listen: %d   %d\n", _sock, _cb);
}

//////////////////////////////////////////////////
// The socket should not be listened to anymore.
void EventScheduler::deletesocket(Socket _sock) {
  FD_CLR(_sock, &active_fd_set);
  fdcb.erase(_sock);
}

//////////////////////////////////////////////////
// main loop, listening to events and calling callbacks.
// It does not return.
// Todo: optionally returning when no socket is listened to anymore.
void EventScheduler::loop() {
  struct timeval tv = {1, 0};
  for (;;) {
    if (tv.tv_sec == 0 && tv.tv_usec == 0) {
//      cout << "time event" << endl;
      tv.tv_sec = 1;
    }
    read_fd_set = active_fd_set;
    int sel = select(FD_SETSIZE, &read_fd_set, NULL, NULL, &tv);
    if (sel < 0) {
      perror("select");
      exit(-1);
    }
//    cout << "loop ";
    for (Socket i = 0; sel && i < FD_SETSIZE; i++) {
      if (FD_ISSET(i, &read_fd_set) > 0) { // if there was input:
	sel--;
	std::cout << "socket event " << i << std::endl;
	(*fdcb[i])(i);  // call the callback
      }
    }
  }
}
