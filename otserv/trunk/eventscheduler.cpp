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
// Revision 1.10  2003/09/17 16:35:08  tliffrag
// added !d command and fixed lag on windows
//
// Revision 1.9  2003/08/25 21:28:12  tliffrag
// Fixed all warnings.
//
// Revision 1.8  2003/08/25 21:05:51  tliffrag
// Fixed bugs preventing otserv from running on windows.
//
// Revision 1.7  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.6  2002/05/28 13:55:56  shivoc
// some minor changes
//
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
extern Map map;

EventScheduler::EventScheduler() {
    FD_ZERO(&active_fd_set);
    FD_ZERO(&read_fd_set);
}

//////////////////////////////////////////////////
// If an open socket should be listened to, call this.
void EventScheduler::newsocket(Socket _sock, unary_functor<Socket,void> *_cb) {
    FD_SET(_sock, &active_fd_set);
    fdcb[_sock] = _cb;
    printf("new socket to listen: %d   %d\n", _sock,(int) _cb);
}

//////////////////////////////////////////////////
// The socket should not be listened to anymore.
void EventScheduler::deletesocket(Socket _sock) {
    FD_CLR(_sock, &active_fd_set);
	close(_sock);
    fdcb.erase(_sock);
}

int EventScheduler::addMapTick(int ms){
	stick* t=new stick;
	t->type=TICK_MAP;
	tickList.insert(pair<double, stick*>(((double)ms/1000+getNow()), t));
	return true;
}
int EventScheduler::addCreatureTick(long c, int ms){
	stick* t=new stick;
	t->type=TICK_MAP;
	t->cid=c;
	tickList.insert(pair<double, stick*>(((double)ms/1000+getNow()), t));
	return true;
}

//////////////////////////////////////////////////
// main loop, listening to events and calling callbacks.
// It does not return.
// Todo: optionally returning when no socket is listened to anymore.
void EventScheduler::loop() {
    struct timeval tv = {0, 50000};
    for (;;) {
        if (tv.tv_sec == 0 && tv.tv_usec == 0) {
	//cout << "time event" << endl;
	//finally another 50k microsecs have passed and we tick
	double now=getNow();
	std::multimap<double, stick*, cmpdouble>::iterator i=tickList.lower_bound(now);
	if(i==tickList.begin() || tickList.size()==0);
	else{
	i--;
		while(i!= tickList.begin()){
			stick* t=i->second;
			switch(t->type){
				case TICK_MAP:
					map.tick(now);
					break;
				case TICK_CREATURE:
					map.getPlayerByID(t->cid)->tick(now);
					break;
			}
		}
	}
	    tv.tv_sec = 0; tv.tv_usec=50000;
        }
        read_fd_set = active_fd_set;
        int sel = select(FD_SETSIZE, &read_fd_set, NULL, NULL, &tv);
        if (sel < 0) {
            perror("select");
            exit(-1);
        }
       //#ifdef__DEBUG__ std::cout << "loop " << std::endl; #endif
        if(sel)
        for (Socket i = 0; sel && i < FD_SETSIZE; i++) {
            #ifdef __WINDOWS__
            if (FD_ISSET(active_fd_set.fd_array[i], &read_fd_set)) { // if there was input:
            Socket i_= active_fd_set.fd_array[i];
            #else
            if (FD_ISSET(i, &read_fd_set)) { // if there was input:
	      Socket i_= i;
            #endif
                sel--;
                #ifdef __DEBUG__
		std::cout << "socket event on socket " << i_ << std::endl;
		#endif
                (*fdcb[i_])(i_);  // call the callback
            }
        }
    }
}
