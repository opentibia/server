//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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


#ifndef __OTSERV_OTTHREAD_H__
#define __OTSERV_OTTHREAD_H__

#include "logger.h"

#include <list>
#include <vector>
#include <algorithm>

typedef std::vector< std::pair<uint32_t, uint32_t> > IPList;

#include <boost/thread.hpp>

#define OTSYS_CREATE_THREAD(a, b) 				boost::thread(boost::bind(&a, (void*)b))

#define OTSYS_THREAD_LOCKVAR 					boost::mutex
#define OTSYS_THREAD_LOCKVARINIT(a)
#define OTSYS_THREAD_LOCKVARRELEASE(a)
#define OTSYS_THREAD_LOCK(a, b) 				a.lock();
#define OTSYS_THREAD_UNLOCK(a, b) 				a.unlock();
#define OTSYS_THREAD_UNLOCK_PTR(a, b) 			a->unlock();

#define OTSYS_SLEEP(time)						boost::this_thread::sleep(time)

#define OTSYS_THREAD_TIMEOUT 					false
#define OTSYS_THREAD_SIGNALVAR					boost::timed_mutex
#define OTSYS_THREAD_SIGNALVARINIT(a)
#define OTSYS_THREAD_SIGNAL_SEND(a) 			a.unlock()
inline int OTSYS_THREAD_WAITSIGNAL(OTSYS_THREAD_SIGNALVAR& a, OTSYS_THREAD_LOCKVAR& b) {
	a.lock();
	return (int)true;
}
#define OTSYS_THREAD_WAITSIGNAL_TIMED(a, b, c) 	a.timed_lock(boost::posix_time::milliseconds(c))


#if defined WIN32 || defined __WINDOWS__
#ifdef __WIN_LOW_FRAG_HEAP__
#define _WIN32_WINNT 0x0501
#endif
#include <winsock2.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/timeb.h>

inline int64_t OTSYS_TIME()
{
  _timeb t;
  _ftime(&t);
  return ((int64_t)t.millitm) + ((int64_t)t.time) * 1000;
}

typedef int socklen_t;

#else  // #if defined WIN32 || defined __WINDOWS__

#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <errno.h>

inline int64_t OTSYS_TIME()
{
	timeb t;
	ftime(&t);
	return ((int64_t)t.millitm) + ((int64_t)t.time) * 1000;
}

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef closesocket
#define closesocket close
#endif

#endif // #if defined WIN32 || defined __WINDOWS__

typedef boost::mutex::scoped_lock OTSYS_THREAD_LOCK_CLASS;

#endif // #ifndef __OTSYSTEM_H__
